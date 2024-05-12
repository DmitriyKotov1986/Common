//STL
#include <memory>
#ifndef WIN32
    #include <syslog.h>
#endif

//Qt
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QCoreApplication>

//My
#include "common.h"

#include "tdbloger.h"

using namespace Common;

//static
static TDBLoger *DBLoger_ptr = nullptr;
static const qsizetype MAX_MESSAGE_LENGTH = 4000;

TDBLoger* TDBLoger::DBLoger(const DBConnectionInfo& DBConnectionInfo /* = {} */,
                   const QString& logDBName /* = "Log" */,
                   bool debugMode /* = true */,
                   QObject* parent /* = nullptr */)
{


    if (DBLoger_ptr == nullptr)
    {
        DBLoger_ptr = new TDBLoger(DBConnectionInfo, logDBName, debugMode, parent);
    }

    return DBLoger_ptr;
}

void TDBLoger::deleteDBLoger()
{
    Q_CHECK_PTR(DBLoger_ptr);

    delete DBLoger_ptr;
}

QString TDBLoger::msgCodeToQString(TDBLoger::MSG_CODE code)
{
    switch (code)
    {
    case MSG_CODE::OK_CODE: return "OK";
    case MSG_CODE::INFORMATION_CODE: return "Info";
    case MSG_CODE::WARNING_CODE: return "Warning";
    case MSG_CODE::ERROR_CODE: return "Error";
    case MSG_CODE::CRITICAL_CODE: return "Critical";
    default: return "Undefine";
    };

    return QString();
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
    if (_db.isOpen())
    {
        _db.close();
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
    _db = QSqlDatabase::addDatabase(_dbConnectionInfo.db_Driver, _logDBName);
    _db.setDatabaseName(_dbConnectionInfo.db_DBName);
    _db.setUserName(_dbConnectionInfo.db_UserName);
    _db.setPassword(_dbConnectionInfo.db_Password);
    _db.setConnectOptions(_dbConnectionInfo.db_ConnectOptions);
    _db.setPort(_dbConnectionInfo.db_Port);
    _db.setHostName(_dbConnectionInfo.db_Host);

    //подключаемся к БД
    if (!_db.open())
    {
        _errorString = connectDBErrorString(_db);

        emit errorOccurred(EXIT_CODE::SQL_NOT_CONNECT, _errorString);
    };
}

void TDBLoger::sendLogMsg(Common::TDBLoger::MSG_CODE category, const QString& msg)
{
    Q_ASSERT(_db.isOpen());

    QString shortMsg = msg.size() >= MAX_MESSAGE_LENGTH ? msg.left(MAX_MESSAGE_LENGTH - 1) : msg;
    shortMsg.replace("'", "`");

    if (!_db.isOpen())
    {
        const QString saveMsg = QString("%1>%2").arg(msgCodeToQString(category)).arg(msg);
        _errorString = "Message not save to DB. Log DB is not open";

        qCritical() <<  QString("%1 %2").arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT)).arg(_errorString);
        qCritical() <<  QString("%1 %2").arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT)).arg(saveMsg);

        writeLogFile("NOT SAVE>", saveMsg);

        return;
    }

    if (category == MSG_CODE::CRITICAL_CODE)
    {
        qCritical() <<  QString("%1 %2").arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT)).arg(msg);
        writeLogFile("CRITICAL>", msg);

#ifndef WIN32
        syslog(LOG_CRIT, "%s", msg.toStdString().data());
#endif

    }
    else if (_debugMode)
    {
        qDebug() << QString("%1 %2").arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT)).arg(shortMsg);

#ifndef WIN32
        int logLevel = LOG_INFO;
        switch (category)
        {
        case MSG_CODE::WARNING_CODE:
        {
            logLevel = LOG_WARNING;

            break;
        }
        case MSG_CODE::ERROR_CODE:
        {
            logLevel = LOG_ERR;

            break;
        }
        case MSG_CODE::CRITICAL_CODE:
        {
            logLevel = LOG_CRIT;

            break;
        }
        case MSG_CODE::OK_CODE:
        case MSG_CODE::INFORMATION_CODE:
        default:
        {
            logLevel = LOG_INFO;
        }
        }
        syslog(logLevel, "%s", msg.toStdString().data());
#endif
    }

    if (msg.size() >= MAX_MESSAGE_LENGTH)
    {
        const QString saveMsg = QString("%1>%2").arg(msgCodeToQString(category)).arg(msg);

        writeLogFile("MESSAGE TO LONG>", saveMsg);
    }

    const QString queryText =
        QString("INSERT INTO %1 (CATEGORY, SENDER, MSG) VALUES (%2, '%3', '%4')")
            .arg(_logDBName)
            .arg(QString::number(static_cast<int>(category)))
            .arg(QCoreApplication::applicationName())
            .arg(shortMsg);

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    _db.transaction();
    QSqlQuery query(_db);

    if (!query.exec(queryText))
    {
        _errorString = executeDBErrorString(_db, query);

        writeLogFile("NO_SAVE_LOG_MSG", msg);

        _db.rollback();

        emit errorOccurred(EXIT_CODE::SQL_EXECUTE_QUERY_ERR, _errorString);

        return;
    }

    if (!_db.commit())
    {
        _errorString = commitDBErrorString(_db);

        writeLogFile("NO_SAVE_LOG_MSG", msg);

        emit errorOccurred(EXIT_CODE::SQL_EXECUTE_QUERY_ERR, _errorString);

        return;
    }
}

