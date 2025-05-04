#pragma once

//STL
#include <future>
#include <queue>
#include <memory>

//Qt
#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QCoreApplication>

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
        @param sender - название сервиса отправителя логов
        @param parent - указатель на родительский класс
        @return указатель на глобальный логер. Возвращется гарантированно не nullptr
    */
    static TDBLoger* DBLoger(const DBConnectionInfo& DBConnectionInfo = {},
                             const QString& logDBName = "Log",
                             bool debugMode = true,
                             const QString& sender = QCoreApplication::applicationName(),
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
    ~TDBLoger() override;

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
        @param sender - название сервиса отправителя логов
        @param parent - указатель на родительский класс
     */
    TDBLoger(const DBConnectionInfo& DBConnectionInfo,
             const QString& logDBName,
             bool debugMode,
             const QString& sender,
             QObject* parent = nullptr);

    /*!
        Удаляет устаревшие логи
    */
    void clearOldLog();

    /*!
        Сохраянет очередь запросов к БД в БД
    */
    void saveToDB();

private:
    const DBConnectionInfo _dbConnectionInfo;   ///< Параметры подключения к БД
    const QString _logDBName = "Log";           ///< Название таблицы лога
    const bool _debugMode = true;               ///< Флаг режиа отладки

    QSqlDatabase _db;                           ///< БД

    const QString _sender;                      ///< Название приложение отправителя логов

    std::queue<QString> _queueMessages;         ///< Очередь сообщений к БД

    QString _errorString;                       ///< Текст последней ошибки

    bool _isStarted = false;                    ///< Флаг успешного старта логирования

};

} //namespace Common

Q_DECLARE_METATYPE(Common::TDBLoger::MSG_CODE);
