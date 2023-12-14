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

using namespace Common;

void Common::writeLogFile(const QString& prefix, const QString& msg)
{
    const QString logFileName = QString("./Log/%1.log").arg(QCoreApplication::applicationName()); //имя файла лога

    QDir logDir(QFileInfo(logFileName).absolutePath());
    if (!logDir.exists())
    {
        if (!logDir.mkdir(logDir.absolutePath()))
        {
            qCritical() << QString("%1(!)Cannot make log dir: %2").arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                           .arg(logDir.absolutePath());
            return;
        }
    }

    QFile file(logFileName);
    if (file.open(QFile::WriteOnly | QFile::Append | QFile::Text))
    {
        QByteArray tmp = QString("%1 %2\n%3\n\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(prefix).arg(msg).toUtf8();
        file.write(tmp);
        file.close();
    }
    else
    {
        qCritical() << QString("%1(!) Message not save to log file: %2 Error: %3 Message: %4 %5").arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
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
    if (!db.isOpen())
    {
        QString msg = QString("Cannot query to DB execute because connetion is closed. Query: %1").arg(queryText);
        Common::saveLogToFile(msg);
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);

        exit(EXIT_CODE::SQL_EXECUTE_QUERY_ERR);
    }

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
    Q_ASSERT(db.isValid());

    if (!db.isValid())
    {
        return;
    }

    if (!db.commit())
    {
        QString msg = QString("Cannot commit trancsation. Error: %1").arg(db.lastError().text());
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
        Common::saveLogToFile(msg);

        db.rollback();

        exit(EXIT_CODE::SQL_COMMIT_ERR);
    }
}

void Common::errorDBQuery(QSqlDatabase& db, const QSqlQuery& query)
{
    QString msg = QString("Cannot execute query. Error: %1. Query: %2").arg(query.lastError().text()).arg(query.lastQuery());
    qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
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
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
        Common::saveLogToFile(msg);

        exit(EXIT_CODE::ALREADY_RUNNIG);
    }
}

