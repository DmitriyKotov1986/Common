#pragma once

//Qt
#include <QObject>
#include <QtSql/QSqlDatabase>

//My
#include "common.h"

namespace Common
{

class TDBLoger final
    : public QObject
{
    Q_OBJECT

public:
    enum class MSG_CODE: quint8
    {
        DEBUG_CODE = 0,
        INFORMATION_CODE = 1,
        WARNING_CODE = 2,
        CRITICAL_CODE = 3,
        FATAL_CODE = 4,
    };

public:
    static TDBLoger* DBLoger(const DBConnectionInfo& DBConnectionInfo = {},
                             const QString& logDBName = "Log",
                             bool debugMode = true,
                             QObject* parent = nullptr);

    static void deleteDBLoger();

    static QString msgCodeToQString(MSG_CODE code);

public:
    ~TDBLoger();

    bool isError() const;
    QString errorString();

public slots:
    void start();

private:
    TDBLoger() = delete;
    Q_DISABLE_COPY_MOVE(TDBLoger);

    TDBLoger(const DBConnectionInfo& DBConnectionInfo,
             const QString& logDBName,
             bool debugMode,
             QObject* parent = nullptr);

signals:
    void errorOccurred(Common::EXIT_CODE errorCode, const QString& errorString);

public slots:
    void sendLogMsg(Common::TDBLoger::MSG_CODE category, const QString& msg); //Сохранения логов

private:
    void clearOldLog();

private:
    const DBConnectionInfo _dbConnectionInfo;    
    const QString _logDBName = "Log";
    const bool _debugMode = true;

    QSqlDatabase _db;

    QString _errorString;

    bool _isStarted = false;

};

} //namespace Common

Q_DECLARE_METATYPE(Common::TDBLoger::MSG_CODE);
