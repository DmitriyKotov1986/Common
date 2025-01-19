//Qt
#include <QSqlQuery>
#include <QCoreApplication>
#include <QMutex>
#include <QMutexLocker>

//My
#include "Common/sql.h"

#include "Common/tdbconfig.h"

using namespace Common;

Q_GLOBAL_STATIC(QMutex, configDBMutex);

//static
TDBConfig::TDBConfig(const DBConnectionInfo &DBConnectionInfo, const QString &configDBName, QObject *parent /* = nullptr */)
    : QObject{parent}
    , _dbConnectionInfo(DBConnectionInfo)
    , _configDBName(configDBName)
{
}

TDBConfig::~TDBConfig()
{
    closeDB(_db);
}

bool TDBConfig::isError() const noexcept
{
    return !_errorString.isEmpty();
}

QString TDBConfig::errorString()
{
    const QString result(_errorString);
    _errorString.clear();

    return result;
}

Q_GLOBAL_STATIC(const QString, DEFAULT_VALUE);

const QString& TDBConfig::getValue(const QString &key)
{
    Q_ASSERT(!key.isEmpty());

    QMutexLocker<QMutex> locker(configDBMutex);

    loadFromDB();

    if (!_isLoaded)
    {
        return *DEFAULT_VALUE;
    }

    const auto values_it = _values.find(key);
    if (values_it == _values.end())
    {
        return *DEFAULT_VALUE;
    }

    return values_it->second;
}

void TDBConfig::setValue(const QString &key, const QString &value)
{
    QMutexLocker<QMutex> locker(configDBMutex);

    loadFromDB();

    if (!_isLoaded)
    {
        return;
    }

    auto values_it = _values.find(key);
    if (values_it != _values.end() && value == values_it->second)
    {
        return;
    }

    QString queryText;
    if (_db.driverName() == "QMYSQL")
    {
        if (values_it != _values.end())
        {
            queryText = QString("UPDATE `%1` "
                                "SET `Value` = '%2' "
                                "WHERE `Owner` = '%3' AND `Key` = '%4' ")
                            .arg(_configDBName)
                            .arg(value.toUtf8().toBase64())
                            .arg(QCoreApplication::applicationName())
                            .arg(key);

            values_it->second = value;
        }
        else
        {
            queryText = QString("INSERT INTO `%1` (`Owner`, `Key`, `Value`) "
                                "VALUES('%2', '%3', '%4') ")
                            .arg(_configDBName)
                            .arg(QCoreApplication::applicationName())
                            .arg(key)
                            .arg(value.toUtf8().toBase64());

            _values.insert({key, value});
        }
    }
    else
    {
        if (values_it != _values.end())
        {
            queryText = QString("UPDATE [%1] "
                                "SET [Value] = '%2' "
                                "WHERE [Owner] = '%3' AND [Key] = '%4' ")
                            .arg(_configDBName)
                            .arg(value.toUtf8().toBase64())
                            .arg(QCoreApplication::applicationName())
                            .arg(key);

            values_it->second = value;
        }
        else
        {
            queryText = QString("INSERT INTO [%1] ([Key], [Value]) "
                                "VALUES('%2', '%3', '%4') ")
                            .arg(_configDBName)
                            .arg(QCoreApplication::applicationName())
                            .arg(key)
                            .arg(value.toUtf8().toBase64());

            _values.insert({key, value});
        }
    }

    qDebug() << QString("Value of parametr %1 set %2").arg(key).arg(value);

    try
    {
        DBQueryExecute(_db, queryText);
    }
    catch (const SQLException& err)
    {
        _errorString = err.what();

        emit errorOccurred(EXIT_CODE::SQL_EXECUTE_QUERY_ERR, _errorString);
    } 
}

bool TDBConfig::hasValue(const QString &key)
{
    Q_ASSERT(!key.isEmpty());

    QMutexLocker<QMutex> locker(configDBMutex);

    loadFromDB();

    if (!_isLoaded)
    {
        return false;
    }

    return _values.find(key) != _values.end();
}

void TDBConfig::loadFromDB()
{
    if (_isLoaded)
    {
        return;
    }

    try
    {
        connectToDB(_db, _dbConnectionInfo, _configDBName);
    }
    catch (const SQLException& err)
    {
        _errorString = err.what();

        emit errorOccurred(EXIT_CODE::SQL_NOT_CONNECT, _errorString);

        return;
    }

    QString queryText;
    if (_db.driverName() == "QMYSQL")
    {
        queryText = QString("SELECT `Key`, `Value` "
                            "FROM `%1` "
                            "WHERE `Owner` = '%2'")
                            .arg(_configDBName)
                            .arg(QCoreApplication::applicationName());
    }
    else
    {
        queryText = QString("SELECT [Key], [Value] "
                            "FROM [%1] "
                            "WHERE [Owner] = '%2'")
                            .arg(_configDBName)
                            .arg(QCoreApplication::applicationName());
    }

    try
    {
        QSqlQuery query(_db);
        query.setForwardOnly(true);

        DBQueryExecute(_db, query, queryText);

        while (query.next())
        {
            auto key = query.value("Key").toString();
            auto value = QByteArray::fromBase64(query.value("Value").toString().toUtf8());

            _values.emplace(std::move(key), std::move(value));
        }
    }
    catch (const SQLException& err)
    {
        _errorString = err.what();

        emit errorOccurred(EXIT_CODE::SQL_EXECUTE_QUERY_ERR, _errorString);
    }

    qInfo() << QString("Load config from DB was successfully");

    _isLoaded = true;
}


