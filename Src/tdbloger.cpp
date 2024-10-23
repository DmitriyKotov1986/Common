//STL
#include <memory>

//Qt
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QCoreApplication>

//My
#include "Common/common.h"

#include "Common/tdbloger.h"

using namespace Common;

//static
static TDBLoger *DBLoger_ptr = nullptr;
static const qsizetype MAX_MESSAGE_LENGTH = 1024 * 1024; //1MB
static const qint64 MAX_LOG_SAVE_INTERVAL = 60 * 60 * 24 * 30; //30 days

TDBLoger* TDBLoger::DBLoger(const DBConnectionInfo& DBConnectionInfo /* = {} */,
                   const QString& logDBName /* = "Log" */,
                   bool debugMode /* = true */,
                   QObject* parent /* = nullptr */)
{
    if (!DBLoger_ptr)
    {
        DBLoger_ptr = new TDBLoger(DBConnectionInfo, logDBName, debugMode, parent);
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
                   QObject* parent):
    QObject(parent),
    _dbConnectionInfo(DBConnectionInfo),
    _logDBName(logDBName),
    _debugMode(debugMode)
{
    qRegisterMetaType<Common::TDBLoger::MSG_CODE>();
    qRegisterMetaType<Common::EXIT_CODE>();
}

TDBLoger::~TDBLoger()
{
    if (_isStarted)
    {
        closeDB(_db);

        qInfo() << QString("Logger to DB stopped successfully");
    }
}

bool TDBLoger::isError() const
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

        qInfo() << QString("Logger to DB started successfully. Database: %1. Table: %2").arg(_db.databaseName()).arg(_logDBName);

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
        if (_debugMode)
        {
            qInfo() << msg;
        }
        break;
    case MSG_CODE::WARNING_CODE:
        if (_debugMode)
        {
            qWarning() << msg;
        }
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
        queryText = QString("INSERT INTO `%1` (`DateTime`, `Category`, `Sender`, `Msg`) VALUES (CAST('%2' AS DATETIME), %3, '%4', '%5')")
            .arg(_logDBName)
            .arg(QDateTime::currentDateTime().toString(DATETIME_FORMAT))
            .arg(QString::number(static_cast<int>(category)))
            .arg(QCoreApplication::applicationName())
            .arg(shortMsg);
    }
    else
    {
        queryText = QString("INSERT INTO [%1] ([DateTime], [Category], [Sender], [Msg]) VALUES (CAST('%2' AS DATETIME2), %3, '%4', '%5')")
            .arg(_logDBName)
            .arg(QDateTime::currentDateTime().toString(DATETIME_FORMAT))
            .arg(QString::number(static_cast<int>(category)))
            .arg(QCoreApplication::applicationName())
            .arg(shortMsg);
    }

    try
    {
        DBQueryExecute(_db, queryText);
    }
    catch (const SQLException& err)
    {
        _errorString = err.what();

        emit errorOccurred(EXIT_CODE::SQL_EXECUTE_QUERY_ERR, _errorString);

        const auto saveMsg = QString("%1 %2").arg(msgCodeToQString(category)).arg(msg);

        writeLogFile("ERROR_SAVE_TO_LOG_DB", saveMsg);

        qWarning() << QString("Error save to log DB. Message: %1").arg(msg);
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

