#ifndef COMMON_H
#define COMMON_H

//Qt
#include <QString>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

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
    //LevelGauge
    LEVELGAUGE_UNDEFINE_TYPE = -100, //неизвестный тип устройства
    //SystemMonitor
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

static const QString TIME_FORMAT = "hh:mm:ss.zzz";
static const QString SIMPLY_TIME_FORMAT = "hh:mm:ss";
static const QString DATETIME_FORMAT = "yyyy-MM-dd hh:mm:ss.zzz";

void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

bool checkAlreadyRun();
void exitIfAlreadyRun();

void writeLogFile(const QString& prefix, const QString& msg);
void writeDebugLogFile(const QString& prefix, const QString& msg);
void saveLogToFile(const QString& msg);

bool connectToDB(QSqlDatabase& db, const Common::DBConnectionInfo& connectionInfo, const QString& connectionName);
void DBQueryExecute(QSqlDatabase& db, const QString &queryText); //Выполнят запрос к БД типа INSERT и UPDATE
void DBCommit(QSqlDatabase& db);                                 //выполнят Commit
void errorDBQuery(QSqlDatabase& db, const QSqlQuery& query);

QString connectDBErrorString(const QSqlDatabase& db);
QString executeDBErrorString(const QSqlDatabase& db, const QSqlQuery& query);
QString commitDBErrorString(const QSqlDatabase& db);

} //Common

Q_DECLARE_METATYPE(Common::EXIT_CODE);

#endif // COMMON_H
