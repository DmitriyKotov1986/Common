#include "common.h"

//Qt
#include <QString>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QSystemSemaphore>
#include <QSharedMemory>
#include <QMutex>
#include <QMutexLocker>

using namespace Common;

void Common::writeLogFile(const QString& prefix, const QString& msg)
{
    const QString logFileName = QString("./Log/%1.log").arg(QCoreApplication::applicationName()); //имя файла лога

    static QMutex mutex;
    auto locker = QMutexLocker(&mutex);

    QDir logDir(QFileInfo(logFileName).absolutePath());
    if (!logDir.exists())
    {
        if (!logDir.mkdir(logDir.absolutePath()))
        {
            qCritical() << QString("%1 (!)Cannot make log dir: %2").arg(QDateTime::currentDateTime().toString(TIME_FORMAT))
                           .arg(logDir.absolutePath());
            return;
        }
    }

    QFile file(logFileName);
    if (file.open(QFile::WriteOnly | QFile::Append | QFile::Text))
    {
        QByteArray tmp = QString("%1 %2\n%3\n\n").arg(QDateTime::currentDateTime().toString(DATETIME_FORMAT)).arg(prefix).arg(msg).toUtf8();
        file.write(tmp);
        file.close();
    }
    else
    {
        qCritical() << QString("%1 (!) Message not save to log file: %2 Error: %3 Message: %4 %5").arg(QDateTime::currentDateTime().toString(TIME_FORMAT))
                       .arg(logFileName).arg(file.errorString()).arg(prefix).arg(msg);
    }
}

void Common::writeDebugLogFile(const QString& prefix, const QString& msg) {
    #ifdef QT_DEBUG
        writeLogFile(prefix, msg);
    #endif
}

void Common::saveLogToFile(const QString& msg)
{
    writeLogFile("LOG>", msg);
}

void Common::DBQueryExecute(QSqlDatabase& db, const QString &queryText)
{
    writeDebugLogFile(QString("QUERY TO %1>").arg(db.connectionName()), queryText);

    db.transaction();
    QSqlQuery query(db);

    if (!query.exec(queryText))
    {
        Common::errorDBQuery(db, query);
    }

    DBCommit(db);
}

void Common::DBCommit(QSqlDatabase& db)
{
    Q_ASSERT(db.isOpen());

    if(!db.isOpen())
    {
        return;
    }

    if (!db.commit())
    {
        QString msg = commitDBErrorString(db);
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString(TIME_FORMAT)).arg(msg);
        Common::saveLogToFile(msg);

        db.rollback();

        exit(EXIT_CODE::SQL_COMMIT_ERR);
  }
}

void Common::errorDBQuery(QSqlDatabase& db, const QSqlQuery& query)
{
    QString msg = executeDBErrorString(db, query);
    qCritical() << QString("%1 %2").arg(QTime::currentTime().toString(TIME_FORMAT)).arg(msg);
    Common::saveLogToFile(msg);

    db.rollback();

    exit(EXIT_CODE::SQL_EXECUTE_QUERY_ERR);
}


bool Common::checkAlreadyRun()
{
    QSystemSemaphore semaphore(QString("%1CheckAlreadyRunSemaphore").arg(QCoreApplication::applicationName()), 1);  // создаём семафор
    semaphore.acquire(); // Поднимаем семафор, запрещая другим экземплярам работать с разделяемой памятью

    QSharedMemory sharedMemory(QString("%1CheckAlreadyRunSharedMemoryFlag").arg(QCoreApplication::applicationName()));  // Создаём экземпляр разделяемой памяти
    bool isRunning = false;            // переменную для проверки ууже запущенного приложения
    if (sharedMemory.attach()) // пытаемся присоединить экземпляр разделяемой памяти к уже существующему сегменту
    {
        isRunning = true;      // Если успешно, то определяем, что уже есть запущенный экземпляр
    }
    else
    {
        sharedMemory.create(1); // В противном случае выделяем 1 байт памяти
        isRunning = false;     // И определяем, что других экземпляров не запущено
    }
    semaphore.release();        // Опускаем семафор

    return isRunning;
}

void Common::exitIfAlreadyRun()
{
    if (Common::checkAlreadyRun())
    {
        QString msg = QString("Process already running.");
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString(TIME_FORMAT)).arg(msg);
        Common::saveLogToFile(msg);

        exit(EXIT_CODE::ALREADY_RUNNIG);
    }
}

QString executeDBErrorString(const QSqlDatabase& db, const QSqlQuery& query)
{
    return QString("Cannot execute query. Error: %1. Query: %2")
        .arg(query.lastError().text())
        .arg(query.lastQuery());

}

QString connectDBErrorString(const QSqlDatabase &db)
{
    return QString("Cannot connect to database %1. Error: %2")
        .arg(db.connectionName())
        .arg(db.lastError().text());
}

QString commitDBErrorString(const QSqlDatabase &db)
{
    return QString("Cannot commit trancsation in database %1. Error: %1")
        .arg(db.connectionName())
        .arg(db.lastError().text());
}

bool connectToDB(QSqlDatabase& db, const Common::DBConnectionInfo& connectionInfo, const QString& connectionName)
{
    Q_ASSERT(!db.isOpen());
    Q_ASSERT(!connectionInfo.db_DBName.isEmpty());
    Q_ASSERT(!connectionInfo.db_Driver.isEmpty());

    //настраиваем подключение БД
    db = QSqlDatabase::addDatabase(connectionInfo.db_Driver, connectionName);
    db.setDatabaseName(connectionInfo.db_DBName);
    db.setUserName(connectionInfo.db_UserName);
    db.setPassword(connectionInfo.db_Password);
    db.setConnectOptions(connectionInfo.db_ConnectOptions);
    db.setPort(connectionInfo.db_Port);
    db.setHostName(connectionInfo.db_Host);

    //подключаемся к БД
    return db.open();
}
