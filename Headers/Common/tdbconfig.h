#pragma once

//STL
#include <unordered_map>

//Qt
#include <QObject>
#include <QSqlDatabase>

#include "Common/common.h"
#include "Common/sql.h"

namespace Common
{

///////////////////////////////////////////////////////////////////////////////
///     The TDBConfig class - класс обеспечивает чтение и сохранение параметров
///         приложения в БД. Подключение и считывание параметров из БД происодит
///         при первом вызове методов getValue(...), setValue(...) или hasValue(...)
///
class TDBConfig final
    : public QObject
{
    Q_OBJECT

public:
    /*!
        Конструтор. Планируется использовать толко этот конструктор.
        @param  - параметры подключения к БД
        @param configDBName - имя таблицы в которой хранятся параметры
        @param parent - указатель на родительский класс
    */
    explicit TDBConfig(const Common::DBConnectionInfo& DBConnectionInfo,
                       const QString& configDBName  = "Config",
                       QObject* parent = nullptr);

    /*!
        Деструткор
    */
    ~TDBConfig();

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

    /*!
        Возвращает значение параметра. Этот метод потокобезопасный
        @param key - имя параметра. Не должно быть пустой строкой
        @return значение параметра
    */
    const QString& getValue(const QString& key);

    /*!
        Сохраняет значение параметра. Этот метод потокобезопасный
        @param key - имя параметра
        @param value - значение параметра
    */
    void setValue(const QString& key, const QString& value);

    /*!
        Возвращает true если параметр с именем key существует. Этот метод потокобезопасный
        @param key - имя параметра
        @return - true если параметр с именем key существует
     */
    bool hasValue(const QString& key);

signals:
    /*!
        Сигнал генерируется если в процессе работы с БД произошла ошибка. Этот метод потокобезопасный
        @param errorCode - код ошибки
        @param errorString - текстовое описание ошибки
    */
    void errorOccurred(Common::EXIT_CODE errorCode, const QString& errorString);

private:
    //Удаляем неиспользуемые конструкторы
    TDBConfig() = delete;
    Q_DISABLE_COPY_MOVE(TDBConfig);

    /*!
        Считывает параметры из БД
    */
    void loadFromDB();

private:
    const Common::DBConnectionInfo _dbConnectionInfo;   ///< Параметры подключения к БД
    const QString _configDBName = "Config";             ///< Имя таблицы с параметрами

    QSqlDatabase _db; ///< Подключение к БД

    std::unordered_map<QString, QString> _values; ///< Карта параметров. Ключ - начвание параметра, Значение - значение параметра

    bool _isLoaded = false; ///< Признак загруженности параметров из БД

    QString _errorString;   ///< Строка с описанием ошибки
};

} //namespace Common

