//STL
#include <limits>

//My
#include "Common/common.h"

#include "Common/parser.h"

using namespace Common;

QString Common::XMLReadString(QXmlStreamReader& XMLReader, const QString& tagName, bool mayBeEmpty, qsizetype maxLength)
{
    const auto strValue = XMLReader.readElementText();

    if (!mayBeEmpty && strValue.isEmpty())
    {
        throw ParseException(QString("Invalid value of tag (%1). Value cannot be empty")
                                 .arg(tagName));
    }

    if (strValue.length() > maxLength)
    {
        throw ParseException(QString("Invalid value of tag (%1). Value: %2. The value must be a string no longer than %3 characters")
                                 .arg(tagName)
                                 .arg(strValue)
                                 .arg(maxLength));
    }

    return strValue;
}

bool Common::XMLReadBool(QXmlStreamReader &XMLReader, const QString &tagName)
{
    const auto strValue = XMLReader.readElementText().toLower();

    if (strValue == "yes" || strValue == "1" || strValue == "true")
    {
        return true;
    }
    else
        if (strValue == "no" || strValue == "0" || strValue == "false")
        {
            return false;
        }

    throw ParseException(QString("Invalid value of tag (%1). The value must be boolean (yes/no, true/false, 1/0). Value: %2")
                             .arg(tagName)
                             .arg(strValue));
}

QDateTime Common::XMLReadDateTime(QXmlStreamReader &XMLReader, const QString &tagName, const QString &format)
{
    const auto strValue = XMLReader.readElementText();
    const auto dateTime = QDateTime::fromString(strValue, format);

    if (!dateTime.isValid())
    {
        throw ParseException(QString("Invalid value of tag (%1). The value must be Date/Time on format: %2. Value: %3")
                                 .arg(tagName)
                                 .arg(format)
                                 .arg(strValue));
    }

    return dateTime;
}

double Common::JSONReadNumber(const QJsonObject& json, const QString& key, double minValue /* = std::numeric_limits<double>::min() */, double maxValue /* = std::numeric_limits<double>::max() */)
{
    if (!json.contains(key))
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    const auto& value = json[key];
    if (!value.isDouble())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not numeric: %2").arg(key).arg(value.toVariant().toString()));
    }

    const auto result = value.toDouble();
    if (result < minValue || result > maxValue)
    {
        throw ParseException(QString("Invalid value of key (%1). Value: %2. Value must be unsigned number between [%3, %4]")
                                 .arg(key)
                                 .arg(result)
                                 .arg(minValue)
                                 .arg(maxValue));
    }

    return result;
}

QString Common::JSONReadString(const QJsonObject& json, const QString& key, bool mayBeEmpty /* = true */, qsizetype maxLength /* = std::numeric_limits<qsizetype>::max() */)
{
    if (!json.contains(key))
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    const auto& value = json[key];
    if (!value.isString())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not string: %2").arg(key).arg(value.toVariant().toString()));
    }

    const auto result = value.toString();
    if (!mayBeEmpty && result.isEmpty())
    {
        throw ParseException(QString("Invalid value of key (%1). Value cannot be empty").arg(key));
    }

    if (result.length() > maxLength)
    {
        throw ParseException(QString("Invalid value of key (%1). Value: %2. The value must be a string no longer than %3 characters")
                                 .arg(key)
                                 .arg(result)
                                 .arg(maxLength));
    }

    return result;
}

bool Common::JSONReadBool(const QJsonObject& json, const QString& key)
{
    if (!json.contains(key))
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    const auto& value = json[key];
    if (!value.isBool())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not bool: %2").arg(key).arg(value.toVariant().toString()));
    }

    return value.toBool();
}

QDateTime Common::JSONReadDateTime(const QJsonObject& json, const QString& key, const QString& format /* = Common::DATETIME_FORMAT */)
{
    if (!json.contains(key))
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    const auto& value = json[key];
    if (!value.isString())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not Date/Time: %2").arg(key).arg(value.toVariant().toString()));
    }

    const auto resultStr = value.toString();
    const auto dateTime = QDateTime::fromString(resultStr, format);

    if (!dateTime.isValid())
    {
        throw ParseException(QString("Invalid value of key (%1). The value must be Date/Time on format: %2. Value: %3")
                                 .arg(key)
                                 .arg(format)
                                 .arg(resultStr));
    }

    return dateTime;
}
