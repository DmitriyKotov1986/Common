//STL
#include <functional>

//Qt
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QCoreApplication>

//My
#include "Common/common.h"
#include "Common/sql.h"

#include "Common/tdbloger.h"

using namespace Common;

//static
static TDBLoger *DBLoger_ptr = nullptr;
static const qsizetype MAX_MESSAGE_LENGTH = 1024 * 1024; //1MB
static const qint64 MAX_LOG_SAVE_INTERVAL = 60 * 60 * 24 * 30; //30 days
static const qsizetype MAX_LOG_MESSAGE_COUNT = 10;

TDBLoger* TDBLoger::DBLoger(const DBConnectionInfo& DBConnectionInfo /* = {} */,
                   const QString& logDBName /* = "Log" */,
                   bool debugMode /* = true */,
                   const QString& sender,
                   QObject* parent /* = nullptr */)
{
    if (!DBLoger_ptr)
    {
        DBLoger_ptr = new TDBLoger(DBConnectionInfo, logDBName, debugMode, sender, parent);
    }

    return DBLoger_ptr;
}

void TDBLoger::deleteDBLoger()
{
    delete DBLoger_ptr;

    DBLoger_ptr = nullptr;
}

QString TDBLoger::msgCodeToQString(TDBLoger::MSG_CODE code)
{
    switch (code)
    {
    case MSG_CODE::DEBUG_CODE: return "DEBUG";
    case MSG_CODE::INFORMATION_CODE: return "INFO";
    case MSG_CODE::WARNING_CODE: return "WARNING";
    case MSG_CODE::FATAL_CODE: return "FATAL";
    case MSG_CODE::CRITICAL_CODE: return "CRITICAL";
    default:
        Q_ASSERT(false);
    };

    return "UNDEFINE";
}

//class
TDBLoger::TDBLoger(const DBConnectionInfo& DBConnectionInfo,
                   const QString& logDBName,
                   bool debugMode,
                   const QString& sender,
                   QObject* parent)
    : QObject(parent)
    , _dbConnectionInfo(DBConnectionInfo)
    , _logDBName(logDBName)
    , _debugMode(debugMode)
    , _sender(sender)
{
    qRegisterMetaType<Common::TDBLoger::MSG_CODE>();
    qRegisterMetaType<Common::EXIT_CODE>();
}

TDBLoger::~TDBLoger()
{
    if (!_isStarted)
    {
        return;
    }

    saveToDB();

    closeDB(_db);

    const auto msg = QString("Logger to DB stopped successfully");

    writeLogFile("LOGER", msg);

    qInfo() << msg;
}

bool TDBLoger::isError() const noexcept
{
    return !_errorString.isEmpty();
}

QString TDBLoger::errorString()
{
    const QString result(_errorString);
    _errorString.clear();

    return result;
}

void TDBLoger::start()
{
    try
    {
        connectToDB(_db, _dbConnectionInfo, _logDBName);

        const auto msg = QString("Logger to DB started successfully. Database: %1. Table: %2").arg(_db.databaseName()).arg(_logDBName);

        writeLogFile("LOGER", msg);

        qInfo() << msg;

        clearOldLog();

        _isStarted = true;
    }
    catch (const SQLException& err)
    {
        _errorString = err.what();

        emit errorOccurred(EXIT_CODE::SQL_NOT_CONNECT, _errorString);
    }
}

void TDBLoger::sendLogMsg(Common::TDBLoger::MSG_CODE category, const QString& msg)
{
    Q_ASSERT(_db.isOpen());
    Q_ASSERT(_isStarted);

    QString shortMsg = msg.size() >= MAX_MESSAGE_LENGTH ? msg.left(MAX_MESSAGE_LENGTH - 1) : msg;
    shortMsg.replace(QChar(0x27), '`');

    switch (category)
    {
    case MSG_CODE::DEBUG_CODE:
        if (_debugMode)
        {
            qDebug() << msg;
        }
        break;
    case MSG_CODE::INFORMATION_CODE:
        qInfo() << msg;
        break;
    case MSG_CODE::WARNING_CODE:
        qWarning() << msg;
        break;
    case MSG_CODE::CRITICAL_CODE:
        qCritical() << msg;
        break;
    case MSG_CODE::FATAL_CODE:
        qCritical() << msg;
        break;
    default:
        Q_ASSERT(false);
    }

    if (msg.size() >= MAX_MESSAGE_LENGTH)
    {
        const auto saveMsg = QString("%1 %2").arg(msgCodeToQString(category)).arg(msg);

        writeLogFile("MESSAGE_TO_LONG", saveMsg);

        qWarning() << QString("Message too long for save to DB. Message: %1").arg(msg);
    }

    QString queryText;
    if (_db.driverName() == "QMYSQL")
    {
        queryText = QString("INSERT DELAYED INTO `%1` (`DateTime`, `Category`, `Sender`, `Msg`) VALUES (CAST('%2' AS DATETIME), %3, '%4', '%5')")
                        .arg(_logDBName)
                        .arg(QDateTime::currentDateTime().toString(DATETIME_FORMAT))
                        .arg(QString::number(static_cast<int>(category)))
                        .arg(_sender)
                        .arg(shortMsg);
    }
    else
    {
        queryText = QString("INSERT INTO [%1] ([DateTime], [Category], [Sender], [Msg]) VALUES (CAST('%2' AS DATETIME2), %3, '%4', '%5')")
                        .arg(_logDBName)
                        .arg(QDateTime::currentDateTime().toString(DATETIME_FORMAT))
                        .arg(QString::number(static_cast<int>(category)))
                        .arg(_sender)
                        .arg(shortMsg);
    }

     _queueMessages.emplace(std::move(queryText));

     if (_queueMessages.size() >= MAX_LOG_MESSAGE_COUNT)
     {
         saveToDB();
     }
}

void TDBLoger::clearOldLog()
{
    Q_ASSERT(_db.isOpen());

    const auto lastLog = QDateTime::currentDateTime().addSecs(-MAX_LOG_SAVE_INTERVAL);

    QString queryText;
    if (_db.driverName() == "QMYSQL")
    {
        queryText = QString("DELETE FROM `%1` "
                            "WHERE `DateTime` < CAST('%2' AS DATETIME)")
                        .arg(_logDBName)
                        .arg(lastLog.toString(DATETIME_FORMAT));
    }
    else
    {
        queryText = QString("DELETE FROM [%1] "
                            "WHERE [DateTime] < CAST('%2' AS DATETIME2)")
                        .arg(_logDBName)
                        .arg(lastLog.toString(DATETIME_FORMAT));
    }

    DBQueryExecute(_db, queryText);

    qDebug() << QString("Cleared logs before: %1").arg(lastLog.toString(SIMPLY_DATETIME_FORMAT));
}

void TDBLoger::saveToDB()
{
    try
    {
        transactionDB(_db);
        QSqlQuery query(_db);

        while (!_queueMessages.empty())
        {
            const auto& queryText = _queueMessages.front();
            DBQueryExecute(_db, query, queryText);
            _queueMessages.pop();
        }

        commitDB(_db);
    }
    catch (const SQLException& err)
    {
        _errorString = err.what();
        writeLogFile("ERROR_SAVE_TO_LOG_DB", _errorString);

        qCritical() << QString("Error save to log DB. Message: %1").arg(_errorString);

        emit errorOccurred(EXIT_CODE::SQL_EXECUTE_QUERY_ERR,  _errorString);
    }
}

