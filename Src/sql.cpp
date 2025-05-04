//Qt
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlError>
#include <QDateTime>

#include "Common/sql.h"

using namespace Common;

Q_GLOBAL_STATIC(QMutex, connectDBMutex);

void Common::connectToDB(QSqlDatabase& db, const Common::DBConnectionInfo& connectionInfo, const QString& connectionName)
{
    Q_ASSERT(!db.isOpen());
    Q_ASSERT(!connectionInfo.dbName.isEmpty());
    Q_ASSERT(!connectionInfo.driver.isEmpty());

#ifdef QT_DEBUG
    qDebug() << QString("Connect to DB %1:%2").arg(connectionInfo.dbName).arg(connectionName);
#endif
    QMutexLocker<QMutex> connectDBMutexLocker(connectDBMutex);

    //настраиваем подключение БД
    db = QSqlDatabase::addDatabase(connectionInfo.driver, connectionName);
    db.setDatabaseName(connectionInfo.dbName);
    db.setUserName(connectionInfo.userName);
    db.setPassword(connectionInfo.password);
    db.setConnectOptions(connectionInfo.connectOptions);
    db.setPort(connectionInfo.port);
    db.setHostName(connectionInfo.host);

    //подключаемся к БД
    if (!db.open())
    {
        QSqlDatabase::removeDatabase(connectionName);

        connectDBMutexLocker.unlock();

        throw SQLException(connectDBErrorString(db));
    }
}

void Common::closeDB(QSqlDatabase &db)
{
    if (!db.isValid())
    {
        return;
    }

#ifdef QT_DEBUG
    qDebug() << QString("Close DB %1:%2").arg(db.databaseName()).arg(db.connectionName());
#endif

    if (db.isOpen())
    {
        db.close();
    }

    QMutexLocker<QMutex> connectDBMutexLocker(connectDBMutex);

    QSqlDatabase::removeDatabase(db.connectionName()); 
}

void Common::transactionDB(QSqlDatabase &db)
{
#ifdef QT_DEBUG
    qDebug() << QString("Start transaction DB %1:%2").arg(db.databaseName()).arg(db.connectionName());
#endif

    if (!db.transaction())
    {
        if (db.isOpen())
        {
            db.close();
        }

        if (!db.open())
        {
            throw SQLException(transactionDBErrorString(db));
        }
    }
}

void Common::DBQueryExecute(QSqlDatabase& db, const QString &queryText)
{
    Q_ASSERT(db.isOpen());

    transactionDB(db);

#ifdef QT_DEBUG
    qDebug() << QString("Query to DB %1:%2: %3").arg(db.databaseName()).arg(db.connectionName()).arg(queryText);
#endif

    QSqlQuery query(db);
    if (!query.exec(queryText))
    {
        db.rollback();

        throw SQLException(executeDBErrorString(db, query));
    }

    commitDB(db);
}

void Common::DBQueryExecute(QSqlDatabase &db, QSqlQuery &query, const QString &queryText)
{
    Q_ASSERT(db.isOpen());

#ifdef QT_DEBUG
    qDebug() << QString("Query to DB %1:%2: %3").arg(db.databaseName()).arg(db.connectionName()).arg(queryText);
#endif

    if (!query.exec(queryText))
    {
        throw SQLException(executeDBErrorString(db, query));
    }
}

void Common::commitDB(QSqlDatabase &db)
{
    Q_ASSERT(db.isOpen());

#ifdef QT_DEBUG
    qDebug() << QString("Commit DB %1:%2").arg(db.databaseName()).arg(db.connectionName());
#endif

    if (!db.commit())
    {
        db.rollback();

        throw SQLException(commitDBErrorString(db));
    }
}

QString Common::executeDBErrorString(const QSqlDatabase& db, const QSqlQuery& query)
{
    return QString("Cannot execute query. Databese: %1. Connection name: %2. Error: %3 Query: %4")
        .arg(db.databaseName())
        .arg(db.connectionName())
        .arg(query.lastError().text())
        .arg(query.lastQuery());
}

QString Common::connectDBErrorString(const QSqlDatabase &db)
{
    return QString("Cannot connect to database %1. Connection name: %2. Error: %3")
        .arg(db.databaseName().isEmpty() ? "UNDEFINED" : db.databaseName())
        .arg(db.connectionName().isEmpty() ? "UNDEFINED" : db.connectionName())
        .arg(db.lastError().text());
}

QString Common::commitDBErrorString(const QSqlDatabase &db)
{
    return QString("Cannot commit trancsation in database %1. Connection name: %2. Error: %3")
        .arg(db.databaseName())
        .arg(db.connectionName())
        .arg(db.lastError().text());
}

QString Common::transactionDBErrorString(const QSqlDatabase &db)
{
    return QString("Cannot start transaction in database %1. Connection name: %2. Error: %3")
        .arg(db.databaseName())
        .arg(db.connectionName())
        .arg(db.lastError().text());
}

QString DBConnectionInfo::check() const
{
    const auto driverslist = QSqlDatabase::drivers();
    if (!driverslist.contains(driver))
    {
        return QString("Driver %1 is not support. Supported drivers: %2").arg(driver).arg(driverslist.join(','));
    }
    if (dbName.isEmpty())
    {
        return QString("DB Name cannot be empty");
    }

    return {};
}
