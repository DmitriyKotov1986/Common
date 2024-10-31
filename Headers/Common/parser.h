#pragma once

//STL
#include  <stdexcept>

//Qt
#include <QString>
#include <QXmlStreamReader>
#include <QJsonObject>

namespace Common
{

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

} //namespace Common
