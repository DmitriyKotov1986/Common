#pragma once

//STL
#include  <stdexcept>

//Qt
#include <QString>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

//My
#include "common.h"

namespace Common
{

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

} //namespace Common
