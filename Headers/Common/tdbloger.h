#pragma once

//STL
#include <future>
#include <queue>
#include <memory>

//Qt
#include <QObject>
#include <QtSql/QSqlDatabase>

//My
#include "common.h"
#include "sql.h"

namespace Common
{

///////////////////////////////////////////////////////////////////////////////
///     The TDBLoger class - логер БД. Предполагается что это глобальный сиглтон класс.
///
class TDBLoger final
    : public QObject
{
    Q_OBJECT

public:
    /*!
        Тип собщения логера
     */
    enum class MSG_CODE: quint8
    {
        DEBUG_CODE = 0,         ///< Отладочное сообщение
        INFORMATION_CODE = 1,   ///< Информационное сообщение
        WARNING_CODE = 2,       ///< Предупреждение (что-то пошло не так, но продолжать работу можно)
        CRITICAL_CODE = 3,      ///< Критическая ошибка (продолжать работу нельзя)
        FATAL_CODE = 4          ///< Ошибка в коде (сработал assert)
    };

public:
    /*!
        Реализация синглтона для логера. Создает новый логер, или метод вызван с параметрами, или
            возарщает указатель на ранее созданные, если без параметров. Первый вызов обязательно должен быть
            с параметрами
        @param DBConnectionInfo - конфигурация подключения к БД
        @param logDBName - Название таблицы с логами
        @param debugMode - включит/выключить режим отладки. В режиме отладки в консоль перенаправляются все сообщения,
            поступающие в логер, в обычном режиме - только сообщения об ошибках
        @param parent - указатель на родительский класс
        @return указатель на глобальный логер. Возвращется гарантированно не nullptr
    */
    static TDBLoger* DBLoger(const DBConnectionInfo& DBConnectionInfo = {},
                             const QString& logDBName = "Log",
                             bool debugMode = true,
                             QObject* parent = nullptr);

    /*!
        Удалят логер
    */
    static void deleteDBLoger();

    /*!
        Преобразует код сообщения в строку
        @param code -  код сообщеия
        @return - строковой эквивалент кода сообщения
    */
    static QString msgCodeToQString(MSG_CODE code);

public:
    /*!
        Деструктор
    */
    ~TDBLoger();

    /*!
        Возвращает true если при выполнении последнего действия произошла ошибка
        @return true - если есть ошибка
     */
    bool isError() const noexcept;

    /*!
        Возвращает тектовое описание ошибки и сбразывает ее
        @return - текст ошибки
    */
    [[nodiscard]] QString errorString();

public slots:
    /*!
        Начать работу логера. Этот метод должен быть вызывн до первого вызова sendLogMsg().
            В этом методе происходит подключение к БД. Если не планируется использоавть. В сслучае
            ошибки будет сгенерирован сигнал errorOccurred(...) или isError() вернет true
            ситему Сигнал/Слот, то просто вызовите этот метод.
    */
    void start();

private:
    // Удаляемнеиспользуемые конструторы
    TDBLoger() = delete;
    Q_DISABLE_COPY_MOVE(TDBLoger);

    /*!
        Конструктор панируется использовать только этот конструтор. Конструтор скрыт, т.к. предполагается создание экзепляра класса через
            вызов static TDBLoger* DBLoger(...)
        @param DBConnectionInfo - конфигурация подключения к БД
        @param logDBName - Название таблицы с логами
        @param debugMode - включит/выключить режим отладки. В режиме отладки в консоль перенаправляются все сообщения,
            поступающие в логер, в обычном режиме - только сообщения об ошибках
        @param parent - указатель на родительский класс
     */
    TDBLoger(const DBConnectionInfo& DBConnectionInfo,
             const QString& logDBName,
             bool debugMode,
             QObject* parent = nullptr);

    /*!
        Удаляет устаревшие логи
    */
    void clearOldLog();

    using QueueMessages = std::queue<QString>; ///< Очередь запросов к БД
    using PQueueMessages = std::unique_ptr<QueueMessages>;///< Указатель на очередь запросов к БД

    /*!
        Сохраянет очередь запросов к БД в БД
        @param db ссылка на БД
        @param queueMessages - очередь запросов
        @return nullopt в случае успеха или строку с описанием ошибки
    */
    static std::optional<QString> saveToDB(QSqlDatabase& db, PQueueMessages queueMessages);

    /*!
        Проверяет результаты сохранения очереди сообщений в БД
    */
    void checkResult();

signals:
    /*!
        Сигнал при фатальной ошибка при работе с БД (не удалось подключится или выполнить запрос и т.п.
        @param errorCode - код ошибки
        @param errorString - текстовое описание ошибки
    */
    void errorOccurred(Common::EXIT_CODE errorCode, const QString& errorString);

public slots:
    /*!
        Записывает сообщение в лог. Если сообшение не удалось записать в БД, оно будет сохранено в LOG-файй
            Если не планируется использовать Сигнал/Слот, просто используйте даннй метод как метод
        @param category - категория сообщения
        @param msg - сообщение
     */
    void sendLogMsg(Common::TDBLoger::MSG_CODE category, const QString& msg); //Сохранения логов

private:
    const DBConnectionInfo _dbConnectionInfo;   ///< Параметры подключения к БД
    const QString _logDBName = "Log";           ///< Название таблицы лога
    const bool _debugMode = true;               ///< Флаг режиа отладки

    QSqlDatabase _db;                           ///< БД

    PQueueMessages _queueMessages;              ///<  - очередь сообщений к БД
    std::future<std::optional<QString>> _saveResult;  ///< результат хранения очереди сообщений

    QString _errorString;                       ///< Текст последней ошибки

    bool _isStarted = false;                    ///< Флаг успешного старта логирования

};

} //namespace Common

Q_DECLARE_METATYPE(Common::TDBLoger::MSG_CODE);
