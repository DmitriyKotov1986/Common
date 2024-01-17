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
        emit errorOccurred(EXIT_CODE::SQL_NOT_CONNECT, connectDBErrorString(_db));
    };
}

void TDBLoger::sendLogMsg(TDBLoger::MSG_CODE category, const QString& msg)
{
    if (category == MSG_CODE::CRITICAL_CODE)
    {
        qCritical() <<  QString("%1 %2").arg(QTime::currentTime().toString(TIME_FORMAT)).arg(msg);
        writeLogFile("CRITICAL>", msg);
    }
    else if (_debugMode)
    {
        qDebug() << QString("%1 %2").arg(QTime::currentTime().toString(TIME_FORMAT)).arg(msg);

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

    const QString queryText =
        QString("INSERT INTO %1 (CATEGORY, SENDER, MSG) VALUES (%2, '%3', '%4')")
            .arg(_logDBName)
            .arg(QString::number(static_cast<int>(category)))
            .arg(QCoreApplication::applicationName())
            .arg(msg);

    writeDebugLogFile(QString("QUERY TO %1>").arg(_db.connectionName()), queryText);

    _db.transaction();
    QSqlQuery query(_db);

    if (!query.exec(queryText))
    {
        emit errorOccurred(EXIT_CODE::SQL_EXECUTE_QUERY_ERR, executeDBErrorString(_db, query));
        writeLogFile("NO_SAVE_LOG_MSG", msg);

        _db.rollback();

        return;
    }

    if (!_db.commit())
    {
        errorOccurred(EXIT_CODE::SQL_EXECUTE_QUERY_ERR, commitDBErrorString(_db));
        writeLogFile("NO_SAVE_LOG_MSG", msg);
    }
}

