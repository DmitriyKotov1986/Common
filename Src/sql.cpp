//Qt
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QSqlError>

#include "Common/sql.h"

using namespace Common;

static std::unordered_map<QString, QDateTime> DBConnectionsList;
static std::unique_ptr<QTimer> DBConnectionsTimer_p = nullptr;
Q_GLOBAL_STATIC(QMutex, checkDBConnectionMutex);
static const int MAX_DB_CONNECTION_TIMEOUT = 60 * 10;

void checkDBConnect()
{
    const auto currentDateTime = QDateTime::currentDateTime();

    QMutexLocker<QMutex> checkDBConnectionLocker(checkDBConnectionMutex);
    for (auto& connection: DBConnectionsList)
    {
        if (connection.second.secsTo(currentDateTime) > MAX_DB_CONNECTION_TIMEOUT)
        {
            auto db = QSqlDatabase::database(connection.first);

            if (db.isValid() && db.isOpen())
            {
                try
                {
                    qDebug() << QString("Check DB connection: %1").arg(connection.first);

                    if (!db.transaction())
                    {
                        throw SQLException(transactionDBErrorString(db));
                    }

                    QSqlQuery query(db);

                    if (!query.exec("SELECT 1"))
                    {
                        throw SQLException(executeDBErrorString(db, query));
                    }

                    if (!db.commit())
                    {
                        throw SQLException(commitDBErrorString(db));
                    }

                    connection.second = currentDateTime;
                }
                catch (const SQLException& err)
                {
                    qWarning() << QString("Cannot check DB connection. Error: %1").arg(err.what());
                }
            }
        }
    }
}

Q_GLOBAL_STATIC(QMutex, connectDBMutex);

void Common::connectToDB(QSqlDatabase& db, const Common::DBConnectionInfo& connectionInfo, const QString& connectionName)
{
    Q_ASSERT(!db.isOpen());
    Q_ASSERT(!connectionInfo.db_DBName.isEmpty());
    Q_ASSERT(!connectionInfo.db_Driver.isEmpty());

    qDebug() << QString("Connect to DB %1:%2").arg(connectionInfo.db_DBName).arg(connectionName);

    QMutexLocker<QMutex> connectDBMutexLocker(connectDBMutex);

    //настраиваем подключение БД
    db = QSqlDatabase::addDatabase(connectionInfo.db_Driver, connectionName);
    db.setDatabaseName(connectionInfo.db_DBName);
    db.setUserName(connectionInfo.db_UserName);
    db.setPassword(connectionInfo.db_Password);
    db.setConnectOptions(connectionInfo.db_ConnectOptions);
    db.setPort(connectionInfo.db_Port);
    db.setHostName(connectionInfo.db_Host);

    if (DBConnectionsTimer_p == nullptr)
    {
        DBConnectionsTimer_p = std::make_unique<QTimer>();

        QObject::connect(DBConnectionsTimer_p .get(), &QTimer::timeout, &checkDBConnect);

        DBConnectionsTimer_p->start(60000);
    }

    //подключаемся к БД
    if (!db.open())
    {
        QSqlDatabase::removeDatabase(connectionName);

        connectDBMutexLocker.unlock();

        throw SQLException(connectDBErrorString(db));
    }

    QMutexLocker<QMutex> checkDBConnectionLocker(checkDBConnectionMutex);
    DBConnectionsList.insert({connectionName, QDateTime::currentDateTime()});
}

void Common::closeDB(QSqlDatabase &db)
{
    if (!db.isValid())
    {
        return;
    }

    qDebug() << QString("Close DB %1:%2").arg(db.databaseName()).arg(db.connectionName());

    if (db.isOpen())
    {
        db.close();
    }

    QMutexLocker<QMutex> connectDBMutexLocker(connectDBMutex);

    const auto DBConnectionsList_it = DBConnectionsList.find(db.connectionName());
    if (DBConnectionsList_it != DBConnectionsList.end())
    {
        DBConnectionsList.erase(DBConnectionsList_it);

        if (DBConnectionsList.empty())
        {
            DBConnectionsTimer_p.reset();
        }
    }

    QSqlDatabase::removeDatabase(db.connectionName());
}

void Common::transactionDB(QSqlDatabase &db)
{
    Q_ASSERT(db.isOpen());

    qDebug() << QString("Start transaction DB %1:%2").arg(db.databaseName()).arg(db.connectionName());

    if (!db.transaction())
    {
        throw SQLException(transactionDBErrorString(db));
    }
}

void Common::DBQueryExecute(QSqlDatabase& db, const QString &queryText)
{
    Q_ASSERT(db.isOpen());

    transactionDB(db);

    qDebug() << QString("Query to DB %1:%2: %3").arg(db.databaseName()).arg(db.connectionName()).arg(queryText);

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

    qDebug() << QString("Query to DB %1:%2: %3").arg(db.databaseName()).arg(db.connectionName()).arg(queryText);

    if (!query.exec(queryText))
    {
        throw SQLException(executeDBErrorString(db, query));
    }
}

void Common::commitDB(QSqlDatabase &db)
{
    Q_ASSERT(db.isOpen());

    qDebug() << QString("Commit DB %1:%2").arg(db.databaseName()).arg(db.connectionName());

    if (!db.commit())
    {
        db.rollback();

        throw SQLException(commitDBErrorString(db));
    }

    QMutexLocker<QMutex> checkDBConnectionLocker(checkDBConnectionMutex);
    const auto DBConnectionsList_it = DBConnectionsList.find(db.connectionName());
    if (DBConnectionsList_it != DBConnectionsList.end())
    {
        DBConnectionsList_it->second = QDateTime::currentDateTime();
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
