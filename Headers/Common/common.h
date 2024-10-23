#pragma once

//STL
#include <limits>

//Qt
#include <QString>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QXmlStreamReader>
#include <QJsonObject>

namespace Common
{

enum EXIT_CODE: int
{
    //For all
    OK = 0,                         //успешное завершение
    LOAD_CONFIG_ERR = -1,           //ошибка чтения файла конфигурации
    ALREADY_RUNNIG = -2,            //попытка повторного запуска процесса
    START_LOGGER_ERR = -3,          //ошибка запуска логера
    UNREGISTER_COPY = -4,           //незарегистрированная версия программы
    //SQL
    SQL_EXECUTE_QUERY_ERR = -10,   //ошибка выполнения SQL запроса
    SQL_COMMIT_ERR = -11,          //ошибка выполнения commit
    SQL_NOT_OPEN_DB = -12,         //БД не открыта
    SQL_NOT_CONNECT = -13,
    SQL_START_TRANSACTION_ERR = -14,
    //Service
    SERVICE_INIT_ERR = -200,       //ошибка инициализации сервиса/демона
    SERVICE_START_ERR = -201,
    SERVICE_RESUME_ERR = -202,
    SERVICE_STOP_ERR = -203,
    SERVISE_PAUSE_ERR = -204,
    //XML Parser
    XML_EMPTY = -500,              //XML пустая
    XML_PARSE_ERR = 501,           //XML ошибка парсинга
    XML_UNDEFINE_TOCKEN = 502      //XML Неизвестный токен
};

class StartException
    : public std::runtime_error
{
public:
    StartException(int exitCode, const QString& err)
        : std::runtime_error(err.toStdString())
        , _exitCode(exitCode)
    {}

    int exitCode() const {return _exitCode;};

private:
    StartException() = delete;
    Q_DISABLE_COPY_MOVE(StartException)

private:
    const int _exitCode = EXIT_CODE::OK;
};

struct DBConnectionInfo
{
    QString db_Driver;
    QString db_DBName;
    QString db_UserName;
    QString db_Password;
    QString db_ConnectOptions;
    QString db_Host;
    quint16 db_Port = 0;
};

//форматы дат и времени
static const QString TIME_FORMAT("hh:mm:ss.zzz");
static const QString SIMPLY_TIME_FORMAT("hh:mm:ss");
static const QString DATETIME_FORMAT("yyyy-MM-dd hh:mm:ss.zzz");
static const QString SIMPLY_DATETIME_FORMAT("yyyy-MM-dd hh:mm:ss");

//вспомогательный класс ошибки парсинга
class ParseException
    : public std::runtime_error
{
public:
    ParseException() = delete;

    explicit ParseException(const QString& what)
        : std::runtime_error(what.toStdString())
    {
    }
};

//XML
template <typename TNumber>
TNumber XMLReadNumber(QXmlStreamReader& XMLReader, const QString& tagName, TNumber minValue = std::numeric_limits<TNumber>::min(), TNumber maxValue = std::numeric_limits<TNumber>::max())
{
    const auto strValue = XMLReader.readElementText();
    bool ok = false;

    if constexpr (std::is_same_v<TNumber, quint64> || std::is_same_v<TNumber, quint32> || std::is_same_v<TNumber, quint16> || std::is_same_v<TNumber, quint8>)
    {
        const quint64 result = strValue.toULongLong(&ok);
        if (!ok || result < minValue || result > maxValue)
        {
            throw ParseException(QString("Invalid value of tag (%1). Value: %2. Value must be unsigned number between [%3, %4]")
                                    .arg(tagName)
                                    .arg(strValue)
                                    .arg(minValue)
                                    .arg(maxValue));
        }

        return static_cast<TNumber>(result);
    }

    const auto result = strValue.toLongLong(&ok);
    if (!ok || result < minValue || result > maxValue)
    {
        throw ParseException(QString("Invalid value of tag (%1). Value: %2. Value must be number between [%3, %4]")
                                .arg(tagName)
                                .arg(strValue)
                                .arg(minValue)
                                .arg(maxValue));
    }

    return static_cast<TNumber>(result);
}

QString XMLReadString(QXmlStreamReader& XMLReader, const QString& tagName, bool mayBeEmpty = true, qsizetype maxLength = std::numeric_limits<qsizetype>::max());
bool XMLReadBool(QXmlStreamReader& XMLReader, const QString& tagName);
QDateTime XMLReadDateTime(QXmlStreamReader& XMLReader, const QString& tagName, const QString& format = Common::DATETIME_FORMAT);

//JSON
double JSONReadNumber(const QJsonObject& json, const QString& key, double minValue = std::numeric_limits<double>::min(), double maxValue = std::numeric_limits<double>::max());
QString JSONReadString(const QJsonObject& json, const QString& key, bool mayBeEmpty = true, qsizetype maxLength = std::numeric_limits<qsizetype>::max());
bool JSONReadBool(const QJsonObject& json, const QString& key);
QDateTime JSONReadDateTime(const QJsonObject& json, const QString& key, const QString& format = Common::DATETIME_FORMAT);

//вспомогательный класс ошибки работы с БД
class SQLException
    : public std::runtime_error
{
public:
    SQLException() = delete;

    explicit SQLException(const QString& what)
        : std::runtime_error(what.toStdString())
    {
    }
};

//функция перенаправления отладочных сообщений
void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

//т.к. QFile.errorString() возращает крикозябры - переопределяем эту функцию
QString fileErrorToString(QFileDevice::FileError error);

void writeLogFile(const QString& prefix, const QString& msg);
void writeDebugLogFile(const QString& prefix, const QString& msg);
void saveLogToFile(const QString& msg);

void connectToDB(QSqlDatabase& db, const Common::DBConnectionInfo& connectionInfo, const QString& connectionName);
void closeDB(QSqlDatabase& db);
void transactionDB(QSqlDatabase& db);
void DBQueryExecute(QSqlDatabase& db, const QString &queryText); //Выполнят запрос к БД типа INSERT и UPDATE
void DBQueryExecute(QSqlDatabase& db, QSqlQuery& query, const QString &queryText); //Выполнят запрос к БД типа SELECT
void commitDB(QSqlDatabase& db);

QString connectDBErrorString(const QSqlDatabase& db);
QString executeDBErrorString(const QSqlDatabase& db, const QSqlQuery& query);
QString commitDBErrorString(const QSqlDatabase& db);
QString transactionDBErrorString(const QSqlDatabase& db);

} //Common

Q_DECLARE_METATYPE(Common::EXIT_CODE);
