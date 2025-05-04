#pragma once

//STL
#include <stdexcept>
#include <optional>
#include <limits>
#include <cmath>

//Qt
#include <QString>
#include <QXmlStreamReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>

//My
#include "Common/common.h"

namespace Common
{
///////////////////////////////////////////////////////////////////////////////
///     Вспомогательный класс ошибки парсинга ParseException
///
class ParseException final
    : public std::runtime_error
{
public:
    /*!
        Конструтор. Планируется использовать только такой конструтор
        @param what - пояснительное сообщение об ошибке
    */
    explicit ParseException(const QString& what)
        : std::runtime_error(what.toStdString())
    {
    }

    ~ParseException() override = default;

private:
    // Удаляем неиспользуемые конструторы
    ParseException() = delete;
    Q_DISABLE_COPY_MOVE(ParseException);
};

///////////////////////////////////////////////////////////////////////////////
///     XML. Предполагается слуюющий механизм парсинга: При помощи последовательности вызывов
/// QXmlStreamReader::readNext() считывается файл. В том месте где необходимо считать и проверить значение тега -
/// Вызываются функции парсинга
///////////////////////////////////////////////////////////////////////////////

/*!
    Функция считывает число их XML файла и возвращает его значение. Если в процесе считывания произошла ошибка
        или число не соотвествует заданным критерием - генерируется исключение ParseException.
    @param XMLReader - ссылка на реадер
    @param tagName - название тега внутри котрого находися число. Нужно для диагностики ошибки
    @param minValue - минимально допустимое значение числа. Значение по умолчанию - минимально доступное для данного типа
    @param maxValue- максимально допустимое значение числа. Значение по умолчанию - максимально доступное для данного типа
    @return считаное число
*/
template <typename TNumber>
TNumber XMLReadNumber(QXmlStreamReader& XMLReader, const QString& tagName, TNumber minValue = std::numeric_limits<TNumber>::min(), TNumber maxValue = std::numeric_limits<TNumber>::max())
{
    const auto strValue = XMLReader.readElementText();
    bool ok = false;

    if constexpr (std::is_same_v<TNumber, quint64> || std::is_same_v<TNumber, quint32> || std::is_same_v<TNumber, quint16> || std::is_same_v<TNumber, quint8>)
    {
        const auto result = strValue.toULongLong(&ok);
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

    if constexpr (std::is_same_v<TNumber, qint64> || std::is_same_v<TNumber, qint32> || std::is_same_v<TNumber, qint16> || std::is_same_v<TNumber, qint8>)
    {
        const auto result = strValue.toULongLong(&ok);
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

    const auto result = strValue.toDouble(&ok);
    if (!ok || result < minValue || result > maxValue)
    {
        throw ParseException(QString("Invalid value of tag (%1). Value: %2. Value must be double number between [%3, %4]")
                                 .arg(tagName)
                                 .arg(strValue)
                                 .arg(minValue)
                                 .arg(maxValue));
    }

    return static_cast<TNumber>(result);

}

/*!
    Считывает строку их XML документа. Если в процесе считывания произошла ошибка
        или стока не соотвествует заданным критерием - генерируется исключение ParseException.
    @param XMLReader - ссылка на реадер
    @param tagName - название тега внутри котрого находися число. Нужно для диагностики ошибки
    @param mayBeEmpty - может ли быть считанная строка пустой. Емли значение равно false и строка пустая
        будет сгенерированно усключение ParseException
    @param maxLength - максимальная длина строки. Если считанная строка  длинне
        будет сгенерированно усключение ParseException
    @return считанная строка
*/
QString XMLReadString(QXmlStreamReader& XMLReader, const QString& tagName, bool mayBeEmpty = true, qsizetype maxLength = std::numeric_limits<qsizetype>::max());

/*!
    Считывает буленовое значени XML. Если начение тега равно "yes", "1", "true" метод вернет true,
        если - "no", "0", "false" - false. Любое другое значение считается ошибкой и будет сгенерированно
        исключение ParseException
    @param XMLReader - ссылка на реадер
    @param tagName - название тега внутри котрого находися число. Нужно для диагностики ошибки
    @return  - буленовое значение
*/
bool XMLReadBool(QXmlStreamReader& XMLReader, const QString& tagName);

/*!
    Метод считывает Дату/Время из XML. Если значение не соответствут формату format - метод сгенерирует
        исключение ParseException
    @param XMLReader - ссылка на реадер
    @param tagName - название тега внутри котрого находися число. Нужно для диагностики ошибки
    @param format - формат в котором ожидается Дата/Время
    @return дата/время
*/
QDateTime XMLReadDateTime(QXmlStreamReader& XMLReader, const QString& tagName, const QString& format = Common::DATETIME_FORMAT);

///////////////////////////////////////////////////////////////////////////////
///     JSON парсеры.
///////////////////////////////////////////////////////////////////////////////

/*!
    Считывает число. Если число не соотвествует критериям, указанным в параметрах -
        будет сгененрированно исключение ParseException
    @param json - значение JSON котрое необходимо преобразовать в число
    @param path - путь к значению. Необходимо для диагностики ошибки
    @param minValue - минимально допустимое значение числа. Значение по умолчанию - минимально доступное для данного типа
    @param maxValue- максимально допустимое значение числа. Значение по умолчанию - максимально доступное для данного типа
    @return считаное число
*/
template <typename TNumber>
std::optional<TNumber> JSONReadNumber(const QJsonValue& json, const QString& path, TNumber minValue = std::numeric_limits<TNumber>::min(), TNumber maxValue = std::numeric_limits<TNumber>::max())
{
    if (json.isNull())
    {
        return {};
    }

    if (!json.isDouble() && !json.isString())
    {
        throw ParseException(QString("Value is not number (%1). Value must be number between [%2, %3]")
            .arg(path)
            .arg(minValue)
            .arg(maxValue));
    }

    double result = NAN;
    if (json.isDouble())
    {
        result = static_cast<TNumber>(json.toDouble());
    }
    else
    {
        bool ok = false;
        result = static_cast<TNumber>(json.toString().toDouble(&ok));
        if (!ok)
        {
            throw ParseException(QString("Value is not number (%1). Value must be number between [%2, %3]")
                                     .arg(path)
                                     .arg(minValue)
                                     .arg(maxValue));
        }
    }

    if (result < minValue || result > maxValue)
    {
        throw ParseException(QString("Invalid value (%1). Value must be number between [%2, %3]")
                                 .arg(path)
                                 .arg(minValue)
                                 .arg(maxValue));
    }

    return result;
}

/*!
    Считывает строку из JSON объекта. Если значение не может быть преобразовано в строку - генерируется исключение ParseException
    @param json - значение JSON котрое необходимо преобразовать в строку
    @param path - путь к значению. Необходимо для диагностики ошибки
    @param mayBeEmpty - может ли быть считанная строка пустой. Емли значение равно false и строка пустая
        будет сгенерированно усключение ParseException
    @param maxLength - максимальная длина строки. Если считанная строка  длинне
        будет сгенерированно усключение ParseException
    @return считанная строка
 */
std::optional<QString> JSONReadString(const  QJsonValue& json, const QString& path, bool mayBeEmpty = true, qsizetype maxLength = std::numeric_limits<qsizetype>::max());

/*!
    Считывает буленовое значени из JSON. Если значение не булевое - генерируется исключение ParseException.
    @param json - значение JSON в котром находится значение
    @param key - значение ключа по котору находится значение
    @param path - путь к значению. Необходимо для диагностики ошибки
    @return - буленовое значение или nullopt если значнеи отсутствуте
*/
std::optional<bool> JSONReadBool(const  QJsonValue& json, const QString& path);

/*!
    Считывает Дату/Время из JSON. Если значение содержит ошибку или имеет неверный формат - генерируется исключение ParseException.
    @param json - значение JSON в котром находится значение
    @param key - значение ключа по котору находится хначение
    @param path - путь к значению. Необходимо для диагностики ошибки
    @param format - формат в котором ожидается Дата/Время
    @return - Дата/время или nullopt если значнеи отсутствуте
*/
std::optional<QDateTime> JSONReadDateTime(const QJsonValue& json, const QString& path, const QString& format = Common::DATETIME_FORMAT);

/*!
    Считывает JSON объект из JSON. Если значение содержит ошибку или имеет неверный формат - генерируется исключение ParseException.
    @param json - значение JSON в котром находится значение
    @param key - значение ключа по котору находится хначение
    @param path - путь к значению. Необходимо для диагностики ошибки
    @return - JSON объект
*/
QJsonObject JSONReadMap(const QJsonValue& json, const QString& path);

/*!
    Считывает JSON массив из JSON. Если значение содержит ошибку или имеет неверный формат - генерируется исключение ParseException.
    @param json - значение JSON в котром находится значение
    @param key - значение ключа по котору находится хначение
    @param path - путь к значению. Необходимо для диагностики ошибки
    @return - JSON массивт
*/
QJsonArray JSONReadArray(const QJsonValue& json, const QString& path);

/*!
    Считывает значение числа из JSON объекта. Если значение числа не удовлетворяет заданным критерием - генерируется исключение ParseException.
        Если число отутсвует - функция вернет nullopt
    @param json - значение JSON котрое необходимо преобразовать в число
    @param key - значение ключа по котору находится число
    @param path - путь к значению. Необходимо для диагностики ошибки
    @param minValue - минимально допустимое значение числа. Значение по умолчанию - минимально доступное для данного типа
    @param maxValue- максимально допустимое значение числа. Значение по умолчанию - максимально доступное для данного типа
    @return считаное число или nullopt если значнеи отсутствуте
*/
template <typename TNumber>
std::optional<TNumber> JSONReadMapNumber(const QJsonObject& json, const QString& key, const QString& path, TNumber minValue = std::numeric_limits<TNumber>::min(), TNumber maxValue = std::numeric_limits<TNumber>::max())
{
    const auto it_value = json.find(key);
    if (it_value == json.end())
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(path));
    }

    return JSONReadNumber(*it_value, path, minValue, maxValue);
}

/*!
    Считывает значение строки из JSON объекта. Если значение строки не удовлетворяет заданным критерием - генерируется исключение ParseException.
        Если число отутсвует - функция вернет nullopt
    @param json - значение JSON объект в котром находится значение
    @param key - значение ключа по котору находится число
    @param path - путь к значению. Необходимо для диагностики ошибки
    @param mayBeEmpty - может ли быть считанная строка пустой. Емли значение равно false и строка пустая
        будет сгенерированно усключение ParseException
    @param maxLength - максимальная длина строки. Если считанная строка  длинне
        будет сгенерированно усключение ParseException
    @return считанная строка или nullopt если значнеи отсутствуте
*/
std::optional<QString> JSONReadMapString(const QJsonObject& json, const QString& key, const QString& path, bool mayBeEmpty = true, qsizetype maxLength = std::numeric_limits<qsizetype>::max());

/*!
    Считывает буленовое значени из JSON объекта. Если значение не булевое - генерируется исключение ParseException.
    @param json - значение JSON объект в котром находится значение
    @param key - значение ключа по котору находится значение
    @param path - путь к значению. Необходимо для диагностики ошибки
    @return - буленовое значение или nullopt если значнеи отсутствуте
*/
std::optional<bool> JSONReadMapBool(const QJsonObject& json, const QString& key, const QString& path);

/*!
    Считывает Дату/Время из JSON объекта. Если значение содержит ошибку или имеет неверный формат - генерируется исключение ParseException.
    @param json - значение JSON объект в котром находится значение
    @param key - значение ключа по котору находится хначение
    @param path - путь к значению. Необходимо для диагностики ошибки
    @param format - формат в котором ожидается Дата/Время
    @return - Дата/время или nullopt если значнеи отсутствуте
*/
std::optional<QDateTime> JSONReadMapDateTime(const QJsonObject& json, const QString& key, const QString& path, const QString& format = Common::DATETIME_FORMAT);

/*!
    Считывает JSON объект из JSON объекта. Если значение содержит ошибку или имеет неверный формат - генерируется исключение ParseException.
    @param json - значение JSON объект в котром находится значение
    @param key - значение ключа по котору находится хначение
    @param path - путь к значению. Необходимо для диагностики ошибки
    @return - JSON объект
*/
QJsonObject JSONReadMapToMap(const QJsonObject& json, const QString& key, const QString& path);

/*!
    Считывает JSON массив из JSON объекта. Если значение содержит ошибку или имеет неверный формат - генерируется исключение ParseException.
    @param json - значение JSON объект в котром находится значение
    @param key - значение ключа по котору находится хначение
    @param path - путь к значению. Необходимо для диагностики ошибки
    @return - JSON массивт
*/
QJsonArray JSONReadMapToArray(const QJsonObject& json, const QString& key, const QString& path);

/*!
    Преобразует QJsonDocument в QJsonObject. Если данные невозможно распарсить - генерируется исключение ParseException
    @param json - исходный QJsonDocument
    @return QJsonObject
*/
QJsonObject JSONReadDocumentToMap(const QJsonDocument& json);

/*!
    Преобразует QJsonDocument в QJsonArray. Если данные невозможно распарсить - генерируется исключение ParseException
    @param json - исходный QJsonDocument
    @return QJsonArray
*/
QJsonArray JSONReadDocumentToArray(const QJsonDocument& json);

/*!
    Преобразует набой байт в QJsonDocument. Если данные невозможно распарсить - генерируется исключение ParseException
    @param json - последовательность байт
    @return QJsonDocument
*/
QJsonDocument JSONParseToDocument(const QByteArray& json);

/*!
    Преобразует набой байт QJsonObject. Если данные невозможно распарсить - генерируется исключение ParseException
    @param json - последовательность байт
    @return QJsonObject
*/
QJsonObject JSONParseToMap(const QByteArray& json);

/*!
    Преобразует набой байт QJsonArray. Если данные невозможно распарсить - генерируется исключение ParseException
    @param json - последовательность байт
    @return QJsonArray
*/
QJsonArray JSONParseToArray(const QByteArray& json);

} //namespace Common
