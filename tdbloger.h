#ifndef TDBLOGER_H
#define TDBLOGER_H

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
    enum class MSG_CODE: int
    {
        OK_CODE = 0,
        INFORMATION_CODE = 1,
        WARNING_CODE = 2,
        ERROR_CODE= 3,
        CRITICAL_CODE = 4
    };

public:
    static TDBLoger* DBLoger(const DBConnectionInfo& DBConnectionInfo = {},
                             const QString& logDBName = "Log",
                             bool debugMode = true,
                             QObject* parent = nullptr);

    static void deleteDBLoger();

    static QString msgCodeToQString(MSG_CODE code);

    TDBLoger() = delete;
    TDBLoger(const TDBLoger& ) = delete;
    TDBLoger& operator=(const TDBLoger& ) = delete;
    TDBLoger(const TDBLoger&& ) = delete;
    TDBLoger& operator=(const TDBLoger&& ) = delete;

    ~TDBLoger();

public slots:
    void start();

private:    
    explicit TDBLoger(const DBConnectionInfo& DBConnectionInfo,
                      const QString& logDBName,
                      bool debugMode,
                      QObject* parent);

signals:
    void errorOccurred(Common::EXIT_CODE errorCode, const QString& errorString);

public slots:
    void sendLogMsg(TDBLoger::MSG_CODE category, const QString& msg); //Сохранения логов

private:
    const DBConnectionInfo _dbConnectionInfo;
    QSqlDatabase _db;
    const QString _logDBName = "Log";
    bool _debugMode = true;

};

} //namespace Common

Q_DECLARE_METATYPE(Common::TDBLoger::MSG_CODE);

#endif // TDBLOGER_H
