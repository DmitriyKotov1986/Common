#pragma once

//STL
#include  <stdexcept>

//Qt
#include <QString>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

namespace Common
{

///////////////////////////////////////////////////////////////////////////////
/// Вспомогательный класс ошибки работы с БД
///
class SQLException
    : public std::runtime_error
{
public:
    /*!
        Конструтор. Планируется использовать только этот конструтор
        @param what - описание ошибки
    */
    explicit SQLException(const QString& what)
        : std::runtime_error(what.toStdString())
    {
    }

private:
    // Удаляем неиспользуемые конструторы
    SQLException() = delete;
    Q_DISABLE_COPY_MOVE(SQLException);

};

///////////////////////////////////////////////////////////////////////////////
///     The DBConnectionInfo class параметры подключения к БД
///
struct DBConnectionInfo
{
    QString driver;         ///< Название используемого драйвера (QODBC, QMYSQL, QLITE и т.д.)
    QString dbName;         ///< Название БД
    QString userName;       ///< Имя пользователя
    QString password;       ///< Пароль
    QString connectOptions; ///< Дополнительные опции подключения
    QString host;           ///< Сервер
    quint16 port = 0;       ///< Порт

    /*!
        Возвращает true - если конфигурация соответсвует минимальным требованиям (указан драйвер и имя БД
        @return Пустая строка в случае корректной конфигурации, описание ошибки - в случае наличия ошибок
    */
    QString check() const;
};

/*!
    Выполняет подключение к БД. В случае ошибки возникает исключение SQLException. Подлючение выполненное данным методом
    будет поддерживаться периодиеской отправкой пстого запроса до вызова метода closeDB(). Эта функция потокобезопасна
    @param db - подключение к БД
    @param connectionInfo - Параметры подключения. Должны быть корректными (DBConnectionInfo.check() - возвращает пустую строку)
    @param connectionName - название подключения. должно быть уникальным для прложения
*/
void connectToDB(QSqlDatabase& db, const Common::DBConnectionInfo& connectionInfo, const QString& connectionName);

/*!
    Закрывает подлючение к БД. Эта функция потокобезопасна
    @param db - ссылка на подключение к БД
 */
void closeDB(QSqlDatabase& db);

/*!
    Начинает транзакцию к БД. Если возникнет ошибка - будет сгенерированно исключение SQLException
    @param db - ссылка на подключение к БД
 */
void transactionDB(QSqlDatabase& db);

/*!
    Выполнят запрос к БД типа INSERT, DELETE и UPDATE. Если возникнет ошибка - будет сгенерированно исключение SQLException
    @param db - ссылка на подключение к БД
    @param queryText - текст запроса
*/
void DBQueryExecute(QSqlDatabase& db, const QString &queryText);

/*!
    Выполнят запрос к БД типа SELECT. Если возникнет ошибка - будет сгенерированно исключение SQLException. Часто
        для оптимизации работы следут указать свойство запроса QSqlQuery::setForwardOnly(true)
    @param db - ссылка на подключение к БД
    @param query - ссылка на запрос.
    @param queryText - текст запроса
*/
void DBQueryExecute(QSqlDatabase& db, QSqlQuery& query, const QString &queryText);

/*!
    Завершаеттранзакцию к БД. Если возникнет ошибка - будет сгенерированно исключение SQLException
    @param db - ссылка на подключение к БД
 */
void commitDB(QSqlDatabase& db);

/*!
    Возврщает строку с описанием ошибки подключения к БД
    @param db - ссылка на подключение к БД
    @return строка с описанием ошибки
*/
QString connectDBErrorString(const QSqlDatabase& db);

/*!
    Возврщает строку с описанием ошибки выполнения запроса к БД
    @param db ссылка на подключение к БД
    @return строка с описанием ошибки
*/
QString executeDBErrorString(const QSqlDatabase& db, const QSqlQuery& query);

/*!
    Возврщает строку с описанием ошибки коммита транзакции к БД
    @param db ссылка на подключение к БД
    @return строка с описанием ошибки
*/
QString commitDBErrorString(const QSqlDatabase& db);

/*!
    Возврщает строку с описанием ошибки начала транзакции к БД
    @param db ссылка на подключение к БД
    @return строка с описанием ошибки
*/
QString transactionDBErrorString(const QSqlDatabase& db);

} //namespace Common
