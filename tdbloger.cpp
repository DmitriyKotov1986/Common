#include "tdbloger.h"

//Qt
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QCoreApplication>

//My
#include "common.h"

using namespace Common;

//static
static TDBLoger* DBLogerPtr = nullptr;

TDBLoger* TDBLoger::DBLoger(QSqlDatabase* db, bool debugMode, const QString& logTableName,  QObject* parent)
{
    if (DBLogerPtr == nullptr)
    {
        DBLogerPtr = new TDBLoger(*db, debugMode,logTableName, parent);
    }

    return DBLogerPtr;
}

void TDBLoger::deleteDBLoger()
{
    Q_CHECK_PTR(DBLogerPtr);

    if (DBLogerPtr != nullptr)
    {
        delete DBLogerPtr;

        DBLogerPtr = nullptr;
    }
}

QString TDBLoger::msgCodeToQString(int code)
{
    switch (code)
    {
    case OK_CODE: return "OK";
    case INFORMATION_CODE: return "Info";
    case WARNING_CODE: return "Warning";
    case ERROR_CODE: return "Error";
    case CRITICAL_CODE: return "Critical";
    default: return "Undefine";
    };

    return QString();
}

//class
TDBLoger::TDBLoger(QSqlDatabase& db, bool debugMode /* = true */, const QString& logTableName/* = QString("LOG")*/, QObject* parent /* = nullptr */)
    :  QObject(parent)
    , _db(db)
    , _debugMode(debugMode)
    , _logTableName(logTableName)
{
    Q_ASSERT(_db.isValid());
    Q_ASSERT(!_logTableName.isEmpty());

    //подключаемся к БД
    if ((_db.isOpen()) || (!_db.open()))
    {
        _errorString = QString("Cannot connect to log database. Error: %1").arg(_db.lastError().text());

        return;
    };
}

TDBLoger::~TDBLoger()
{
    if (_db.isOpen())
    {
        _db.close();
    }
}

QString TDBLoger::errorString()
{
    auto res = _errorString;
    _errorString.clear();

    return res;
}

void TDBLoger::sendLogMsg(int category, const QString& msg)
{
    if (category == MSG_CODE::CRITICAL_CODE)
    {
        qCritical() <<  QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
        writeLogFile("CRITICAL>", msg);
    }
    else if (_debugMode)
    {
        qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
    }

    writeDebugLogFile("LOG>", msg);

    QString queryText =
        QString("INSERT INTO %1 (CATEGORY, SENDER, MSG) "
                "VALUES (%2, '%3', '%4')")
            .arg(_logTableName)
            .arg(category)
            .arg(QCoreApplication::applicationName())
            .arg(msg.size() <= 1000 ? msg : msg.first(1000));

    Common::DBQueryExecute(_db, queryText);
}

