#pragma once

//STL
#include <unordered_map>

//Qt
#include <QObject>
#include <QSqlDatabase>

#include "Common/common.h"

namespace Common
{

class TDBConfig final
    : public QObject
{
    Q_OBJECT

public:
    explicit TDBConfig(const DBConnectionInfo& DBConnectionInfo,
                       const QString& configDBName  = "Config",
                       QObject* parent = nullptr);

    ~TDBConfig();

    bool isError() const;
    QString errorString();

    const QString& getValue(const QString& key);
    void setValue(const QString& key, const QString& value);
    bool hasValue(const QString& key);

signals:
    void errorOccurred(Common::EXIT_CODE errorCode, const QString& errorString);

private:
    TDBConfig() = delete;
    Q_DISABLE_COPY_MOVE(TDBConfig);

    void loadFromDB();

private:
    const Common::DBConnectionInfo _dbConnectionInfo;

    const QString _configDBName = "Config";

    QSqlDatabase _db;

    std::unordered_map<QString, QString> _values;

    bool _isLoaded = false;

    QString _errorString;
};

} //namespace Common

