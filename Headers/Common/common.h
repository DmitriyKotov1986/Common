#pragma once

//STL
#include <stdexcept>


//Qt
#include <QString>
#include <QFile>
#include <QDateTime>

namespace Common
{

///////////////////////////////////////////////////////////////////////////////
///     Стандартные коды завершения программ
///
enum EXIT_CODE: int
{
    //For all
    OK = 0,                         ///< успешное завершение
    LOAD_CONFIG_ERR = -1,           ///< ошибка чтения файла конфигурации
    ALREADY_RUNNIG = -2,            ///< попытка повторного запуска процесса
    START_LOGGER_ERR = -3,          ///< ошибка запуска логера
    UNREGISTER_COPY = -4,           ///< незарегистрированная версия программы
    //SQL
    SQL_EXECUTE_QUERY_ERR = -10,    ///< ошибка выполнения SQL запроса
    SQL_COMMIT_ERR = -11,           ///< ошибка выполнения commit
    SQL_NOT_OPEN_DB = -12,          ///< БД не открыта
    SQL_NOT_CONNECT = -13,          ///< Попытка выполнить действие с БД когда соединение не установлкено
    SQL_START_TRANSACTION_ERR = -14,///< Ошибка начала транкзации БД (обычно обзначает что транзакция уже начата)
    //Service
    SERVICE_INIT_ERR = -200,        ///< ошибка инициализации сервиса/демона
    SERVICE_START_ERR = -201,       ///< Ошибка запуска сервиса (при выполнении команды Старт
    SERVICE_RESUME_ERR = -202,      ///< Ошибка перезапуска сервиса
    SERVICE_STOP_ERR = -203,        ///< Ошибка остановки сервиса
    SERVISE_PAUSE_ERR = -204,       ///< Ошибка перехода сервиса в состояние Пауза
    //XML Parser
    XML_EMPTY = -500,               ///< XML пустая
    XML_PARSE_ERR = 501,            ///< XML ошибка парсинга
    XML_UNDEFINE_TOCKEN = 502,      ///< XML Неизвестный токен
    //HTTP_SERVER
    HTTP_SERVER_NOT_LISTEN = 600,   ///< Сервер не может выполнить листинг на порту
    HTTP_SERVER_NOT_LOAD_SSL_CERTIFICATE = 601  ///< Ошибка загрузки SSL сертификата сервером

};

///////////////////////////////////////////////////////////////////////////////
///     The StartException class - исключение при запуске программы (ошибка инициализации)
///
class StartException
    : public std::runtime_error
{
public:
    /*!
        Конструктор. Планируется использовать только этот конструткор
        @param exitCode - код аварийного завершения
        @param err - текстовое описане ошибки
    */
    StartException(int exitCode, const QString& err)
        : std::runtime_error(err.toStdString())
        , _exitCode(exitCode)
    {}

    /*!
        Возвращает код аварийного завершения
        @return код выхода
    */
    int exitCode() const {return _exitCode;};

private:
    //Удаляем неиспользуемые конструкторы
    StartException() = delete;
    Q_DISABLE_COPY_MOVE(StartException)

private:
    const int _exitCode = EXIT_CODE::OK;  ///< Код выхода (аварийного завершения программы

};

//форматы дат и времени
static const QString TIME_FORMAT("hh:mm:ss.zzz");                  ///< Время
static const QString SIMPLY_TIME_FORMAT("hh:mm:ss");               ///< Только время (упрощенный)
static const QString DATETIME_FORMAT("yyyy-MM-dd hh:mm:ss.zzz");   ///< Основной формат даты/времени
static const QString SIMPLY_DATETIME_FORMAT("yyyy-MM-dd hh:mm:ss");///< Дата/время упрощенный

/*!
    Функция перенаправления отладочных сообщений
*/
void messageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

/*!
    Т.к. QFile.errorString() возращает крикозябры - переопределяем эту функцию
*/
QString fileErrorToString(QFileDevice::FileError error);

/*!
    Записывает сообщение в файл в формате "[SIMPLY_TIME_FORMAT] [prefix] [msg]". Имя файла лога определяется как [расположение exe файла]/Log/[название приложения].log
        Если файл или папка не существуют - они будут созданы при первом вызове этой функции. При превышении
        размера файла максимального - файл будет переименован в [название приложения].log_yyyy_MM_dd_hh_mm_ss
    @param prefix - префикс сообщения
    @param msg - сообщение
*/
void writeLogFile(const QString& prefix, const QString& msg);

/*!
    Делает тоже самое что и writeLogFile(...), но только в случае сборки DEBUG
    @param prefix - префикс сообщения
    @param msg - сообщение
*/
void writeDebugLogFile(const QString& prefix, const QString& msg);

/*!
    Сохраняет сообщения лога в файл с помощью writeLogFile(...)
    @param msg - сообщение
*/
void saveLogToFile(const QString& msg);

/*!
    Создает все промежуточные папки для файла fileName
    @param fileName
    @return true - если все подпапки удалось создать или они уже существуют
*/
bool makeFilePath(const QString& fileName);

} //Common

Q_DECLARE_METATYPE(Common::EXIT_CODE);
