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
    #endif
}

void Common::saveLogToFile(const QString& msg)
{
    writeLogFile("LOG", msg);
}

void Common::connectToDB(QSqlDatabase& db, const Common::DBConnectionInfo& connectionInfo, const QString& connectionName)
{
    Q_ASSERT(!db.isOpen());
    Q_ASSERT(!connectionInfo.db_DBName.isEmpty());
    Q_ASSERT(!connectionInfo.db_Driver.isEmpty());

    qDebug() << QString("Connect to DB %1:%2").arg(connectionInfo.db_DBName).arg(connectionName);

    static QMutex connectDBMutex;
    QMutexLocker<QMutex> connectDBMutexLocker(&connectDBMutex);

    //настраиваем подключение БД
    db = QSqlDatabase::addDatabase(connectionInfo.db_Driver, connectionName);
    db.setDatabaseName(connectionInfo.db_DBName);
    db.setUserName(connectionInfo.db_UserName);
    db.setPassword(connectionInfo.db_Password);
    db.setConnectOptions(connectionInfo.db_ConnectOptions);
    db.setPort(connectionInfo.db_Port);
    db.setHostName(connectionInfo.db_Host);

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

    qDebug() << QString("Close DB %1:%2").arg(db.databaseName()).arg(db.connectionName());

    if (db.isOpen())
    {
        db.close();
    }

    QSqlDatabase::removeDatabase(db.connectionName());
}

void Common::transactionDB(QSqlDatabase &db)
{
    Q_ASSERT(db.isOpen());

    //qDebug() << QString("Start transaction DB %1:%2").arg(db.databaseName()).arg(db.connectionName());

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

    //qDebug() << QString("Commit DB %1:%2").arg(db.databaseName()).arg(db.connectionName());

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

void Common::messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
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

QString Common::XMLReadString(QXmlStreamReader& XMLReader, const QString& tagName, bool mayBeEmpty, qsizetype maxLength)
{
    const auto strValue = XMLReader.readElementText();

    if (!mayBeEmpty && strValue.isEmpty())
    {
        throw ParseException(QString("Invalid value of tag (%1). Value cannot be empty")
                                    .arg(tagName));
    }

    if (strValue.length() > maxLength)
    {
        throw ParseException(QString("Invalid value of tag (%1). Value: %2. The value must be a string no longer than %3 characters")
                                    .arg(tagName)
                                    .arg(strValue)
                                    .arg(maxLength));
    }

    return strValue;
}

bool Common::XMLReadBool(QXmlStreamReader &XMLReader, const QString &tagName)
{
    const auto strValue = XMLReader.readElementText().toLower();

    if (strValue == "yes" || strValue == "1" || strValue == "true")
    {
        return true;
    }
    else
    if (strValue == "no" || strValue == "0" || strValue == "false")
    {
        return false;
    }

    throw ParseException(QString("Invalid value of tag (%1). The value must be boolean (yes/no, true/false, 1/0). Value: %2")
                                .arg(tagName)
                                .arg(strValue));
}

QDateTime Common::XMLReadDateTime(QXmlStreamReader &XMLReader, const QString &tagName, const QString &format)
{
    const auto strValue = XMLReader.readElementText();
    const auto dateTime = QDateTime::fromString(strValue, format);

    if (!dateTime.isValid())
    {
        throw ParseException(QString("Invalid value of tag (%1). The value must be Date/Time on format: %2. Value: %3")
                                    .arg(tagName)
                                    .arg(format)
                                    .arg(strValue));
    }

    return dateTime;
}

double Common::JSONReadNumber(const QJsonObject& json, const QString& key, double minValue /* = std::numeric_limits<double>::min() */, double maxValue /* = std::numeric_limits<double>::max() */)
{
    if (!json.contains(key))
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    const auto& value = json[key];
    if (!value.isDouble())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not numeric: %2").arg(key).arg(value.toVariant().toString()));
    }

    const auto result = value.toDouble();
    if (result < minValue || result > maxValue)
    {
        throw ParseException(QString("Invalid value of key (%1). Value: %2. Value must be unsigned number between [%3, %4]")
                                    .arg(key)
                                    .arg(result)
                                    .arg(minValue)
                                    .arg(maxValue));
    }

    return result;
}

QString Common::JSONReadString(const QJsonObject& json, const QString& key, bool mayBeEmpty /* = true */, qsizetype maxLength /* = std::numeric_limits<qsizetype>::max() */)
{
    if (!json.contains(key))
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    const auto& value = json[key];
    if (!value.isString())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not string: %2").arg(key).arg(value.toVariant().toString()));
    }

    const auto result = value.toString();
    if (!mayBeEmpty && result.isEmpty())
    {
        throw ParseException(QString("Invalid value of key (%1). Value cannot be empty").arg(key));
    }

    if (result.length() > maxLength)
    {
        throw ParseException(QString("Invalid value of key (%1). Value: %2. The value must be a string no longer than %3 characters")
                                    .arg(key)
                                    .arg(result)
                                    .arg(maxLength));
    }

    return result;
}

bool Common::JSONReadBool(const QJsonObject& json, const QString& key)
{
    if (!json.contains(key))
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    const auto& value = json[key];
    if (!value.isBool())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not bool: %2").arg(key).arg(value.toVariant().toString()));
    }

    return value.toBool();
}

QDateTime Common::JSONReadDateTime(const QJsonObject& json, const QString& key, const QString& format /* = Common::DATETIME_FORMAT */)
{
    if (!json.contains(key))
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    const auto& value = json[key];
    if (!value.isString())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not Date/Time: %2").arg(key).arg(value.toVariant().toString()));
    }

    const auto resultStr = value.toString();
    const auto dateTime = QDateTime::fromString(resultStr, format);

    if (!dateTime.isValid())
    {
        throw ParseException(QString("Invalid value of key (%1). The value must be Date/Time on format: %2. Value: %3")
                                    .arg(key)
                                    .arg(format)
                                    .arg(resultStr));
    }

    return dateTime;
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

