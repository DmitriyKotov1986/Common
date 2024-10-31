#pragma once

//STL
#include <stdexcept>


//Qt
#include <QString>
#include <QFile>
#include <QDateTime>

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

//функция перенаправления отладочных сообщений
void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

//т.к. QFile.errorString() возращает крикозябры - переопределяем эту функцию
QString fileErrorToString(QFileDevice::FileError error);

void writeLogFile(const QString& prefix, const QString& msg);
void writeDebugLogFile(const QString& prefix, const QString& msg);
void saveLogToFile(const QString& msg);

bool makeFilePath(const QString& fileName);

} //Common

Q_DECLARE_METATYPE(Common::EXIT_CODE);
