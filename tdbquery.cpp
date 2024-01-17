#include "tdbquery.h"

//Qt
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtSql/QSqlRecord>
#include <QMutex>
#include <QMutexLocker>

#include <iostream>

using namespace Common;

TDBQuery::TDBQuery(const DBConnectionInfo& dbConnectionInfo, const QString& queryText, bool selectMode, QObject *parent)
    : QObject(parent)
    , _dbConnectionInfo(dbConnectionInfo)
    , _selectMode(selectMode)
    , _queryText(queryText)
    , _currentConnetionNumber(connectionNumber())
{
    qRegisterMetaType<TDBQuery::TResultRecords>("TDBQuery::TResultRecords");
}

TDBQuery::~TDBQuery()
{
}

void TDBQuery::runQuery()
{
    {
    //настраиваем подключение БД
    QSqlDatabase db = QSqlDatabase::addDatabase(_dbConnectionInfo.db_Driver, QString("DB_%1").arg(_currentConnetionNumber));
    db.setDatabaseName(_dbConnectionInfo.db_DBName);
    db.setUserName(_dbConnectionInfo.db_UserName);
    db.setPassword(_dbConnectionInfo.db_Password);
    db.setConnectOptions(_dbConnectionInfo.db_ConnectOptions);
    db.setPort(_dbConnectionInfo.db_Port);
    db.setHostName(_dbConnectionInfo.db_Host);

    //подключаемся к БД
    if (!db.open())
    {
        exitWithError(QString("Cannot connect to database. Error: %1").arg(db.lastError().text()));

        return;
    };

    db.transaction();
    QSqlQuery query(db);
    query.setForwardOnly(true);

    if (!query.exec(_queryText))
    {
        exitWithError(QString("Cannot execute query. Error: %1. Query: %2").arg(query.lastError().text()).arg(query.lastQuery()));

        return;
    }

    if (_selectMode)
    {
        TResultRecords result;
        const auto record = query.record();
        while (query.next())
        {
            TRow row;
            for (int column = 0; column < record.count(); ++column)
            {
                row.push_back(query.value(column));
            }

            result.push_back(row);
        }

        emit selectResult(result);
    }

    if (!db.commit())
    {
        exitWithError(QString("Cannot commit transaction. Error: %1. Query: %2").arg(query.lastError().text()).arg(query.lastQuery()));

        return;
    }

    //отключаемся от БД
    db.close();
    }

    QSqlDatabase::removeDatabase(QString("DB_%1").arg(_currentConnetionNumber));

    emit finished();
}

void TDBQuery::exitWithError(const QString &msg)
{
    emit errorOccurred(msg);
    emit finished();
}

static int connectionNumberValue = 0;
static QMutex mutex;

int TDBQuery::connectionNumber()
{
    QMutexLocker locker(&mutex);

    return ++connectionNumberValue;
}
