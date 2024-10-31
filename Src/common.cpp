//STL
#include <unordered_map>
#include <memory>

//Qt
#include <QString>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QMutex>
#include <QMutexLocker>
#include <QtSql/QSqlError>
#include <QTextStream>
#include <QTimer>

#include "Common/common.h"

using namespace Common;

static constexpr qsizetype MAX_FILE_LOG_SIZE = 100 * 1024 * 1024; //100 MB
static const int MAX_SAVE_LOG_INTERVAL = 30;

void Common::writeLogFile(const QString& prefix, const QString& msg)
{
    const auto fileInfo = QFileInfo(QString("./Log/%1.log").arg(QCoreApplication::applicationName()));
    const auto fileName = fileInfo.absoluteFilePath();

    static QMutex mutex;
    auto locker = QMutexLocker(&mutex);

    class WriteLogFileException
        : public std::runtime_error
    {
    public:
        WriteLogFileException() = delete;

        explicit WriteLogFileException(const QString& what)
            : std::runtime_error(what.toStdString())
        {
        }
    };

    try
    {
        QDir logDir(fileInfo.absolutePath());
        if (!logDir.exists())
        {
            if (!logDir.mkdir(logDir.absolutePath()))
            {
                 throw WriteLogFileException(QString("Cannot make log dir: %2").arg(logDir.absolutePath()));
            }
        }
        if (fileInfo.exists() && fileInfo.size() > MAX_FILE_LOG_SIZE)
        {

            if (!QFile::rename(fileName, QString("%1_%2").arg(fileName).arg(QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss"))))
            {
                throw WriteLogFileException(QString("Cannot rename old logs to: %1").arg(fileName));
            }

            for (const auto& oldFileName: logDir.entryList())
            {
                QFileInfo oldFileInfo(oldFileName);

                if (!oldFileInfo.fileName().contains(fileInfo.fileName()))
                {
                    continue;
                }

                if (oldFileInfo.lastModified().daysTo(QDateTime::currentDateTime()) > MAX_SAVE_LOG_INTERVAL)
                {
                    QFile oldFile(oldFileName);
                    if (!oldFile.remove())
                    {
                        throw WriteLogFileException(QString("Cannot remove old log file: %1 %2").arg(oldFileInfo.fileName()).arg(fileErrorToString(oldFile.error())));
                    }
                }
            }
        }

        QFile file(fileName);
        if (!file.open(QFile::WriteOnly | QFile::Append | QFile::Text))
        {
            throw WriteLogFileException(QString("Cannot open file to write. %1").arg(fileErrorToString(file.error())));
        }

        QByteArray tmp = QString("%1 %2 %3\n")
                .arg(QDateTime::currentDateTime().toString(DATETIME_FORMAT))
                .arg(prefix)
                .arg(msg).toUtf8();

        if (file.write(tmp) == -1)
        {
             file.close();

             throw WriteLogFileException(QString("Cannot write file. %1").arg(file.errorString()));
        }

        file.close();
    }

    catch (const WriteLogFileException& err)
    {
        QTextStream ss(stderr);
        ss << QString("%1 ERR Message not save to log file: %2. Error: %3. Message: %4 %5\n")
                       .arg(QDateTime::currentDateTime().toString(SIMPLY_TIME_FORMAT))
                       .arg(fileName)
                       .arg(err.what())
                       .arg(prefix)
                       .arg(msg);
    }
}

void Common::writeDebugLogFile(const QString& prefix, const QString& msg)
{
#ifdef QT_DEBUG
    writeLogFile(prefix, msg);
#else
    Q_UNUSED(prefix);
    Q_UNUSED(msg);
#endif
}

void Common::saveLogToFile(const QString& msg)
{
    writeLogFile("LOG", msg);
}


void Common::messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
#ifndef QT_DEBUG
    Q_UNUSED(context);
#endif

    QString stringPrefix;
    bool writeMsgToFile = false;
    switch (type)
    {
    case QtDebugMsg:
        stringPrefix = "DBG"; //debug
        break;
    case QtInfoMsg:
        stringPrefix = "INF"; //info
        break;
    case QtWarningMsg:
        stringPrefix = "WAR"; //warning
        break;
    case QtCriticalMsg:
        stringPrefix = "CRY"; //critical
        writeMsgToFile = true;
        break;
    case QtFatalMsg:
        stringPrefix = "FAT"; //fatal
        writeMsgToFile = true;
        break;
    default:
        stringPrefix = "UND"; //undefine
        writeMsgToFile = true;
        break;
    }
#ifdef QT_DEBUG
    {
        stringPrefix += QString(" %3:%4:%5")
                .arg(context.file)
                .arg(context.line)
                .arg(context.function);

        static QMutex mutex;
        auto locker = QMutexLocker(&mutex);

        QTextStream ss(stderr);
        ss << QString("%1 %2 %3\n")
              .arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT))
              .arg(stringPrefix)
              .arg(msg);
    }
#else
    {
        if (type != QtDebugMsg)
        {
            static QMutex mutex;
            auto locker = QMutexLocker(&mutex);

            QTextStream ss(stderr);
            ss << QString("%1 %2 %3\n").arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT)).arg(stringPrefix).arg(msg);
        }
    }
#endif
    if (writeMsgToFile)
    {
        writeLogFile(stringPrefix, msg);
    }
    else
    {
        writeDebugLogFile(stringPrefix, msg);
    }
}


QString Common::fileErrorToString(QFileDevice::FileError error)
{
    switch (error)
    {
    case QFileDevice::NoError: return "No error occurred";
    case QFileDevice::ReadError: return "An error occurred when reading from the file";
    case QFileDevice::WriteError: return "An error occurred when writing to the file";
    case QFileDevice::FatalError: return "A fatal error occurred";
    case QFileDevice::ResourceError: return "Out of resources (e.g., too many open files, out of memory, etc.)";
    case QFileDevice::OpenError: return "The file could not be opened";
    case QFileDevice::AbortError: return "The operation was aborted";
    case QFileDevice::TimeOutError: return "A timeout occurred";
    case QFileDevice::UnspecifiedError: return "An unspecified error occurred";
    case QFileDevice::RemoveError: return "The file could not be removed";
    case QFileDevice::RenameError: return "The file could not be renamed";
    case QFileDevice::PositionError: return "The position in the file could not be changed";
    case QFileDevice::ResizeError: return "The file could not be resized";
    case QFileDevice::PermissionsError: return "The file could not be accessed";
    case QFileDevice::CopyError: return "The file could not be copied";
    default: break;
    }
    return "Undefine error";
}

bool Common::makeFilePath(const QString& fileName)
{
    if (fileName.isEmpty())
    {
        return false;
    }

    const auto filePath = QFileInfo(fileName).absolutePath();

    QDir dir(filePath);

    return dir.mkpath(filePath);
}
