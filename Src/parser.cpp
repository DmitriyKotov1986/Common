//My
#include "Common/common.h"

#include "Common/parser.h"

using namespace Common;

///////////////////////////////////////////////////////////////////////////////
///     XML
///
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
    else if (strValue == "no" || strValue == "0" || strValue == "false")
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


///////////////////////////////////////////////////////////////////////////////
///     JSON
///
std::optional<QString> Common::JSONReadMapString(const QJsonObject& json, const QString& key, const QString &path, bool mayBeEmpty /* = true */, qsizetype maxLength /* = std::numeric_limits<qsizetype>::max() */)
{
    const auto it_value = json.find(key);
    if (it_value == json.end())
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    return JSONReadString(*it_value, path, mayBeEmpty, maxLength);
}

std::optional<bool> Common::JSONReadMapBool(const QJsonObject& json, const QString& key, const QString &path)
{
    const auto it_value = json.find(key);
    if (it_value == json.end())
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    return JSONReadBool(*it_value, path);
}

std::optional<QDateTime> Common::JSONReadMapDateTime(const QJsonObject& json, const QString& key, const QString &path, const QString& format /* = Common::DATETIME_FORMAT */)
{
    const auto it_value = json.find(key);
    if (it_value == json.end())
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    return JSONReadDateTime(*it_value, path, format);
}

std::optional<QString> Common::JSONReadString(const QJsonValue &json, const QString &path, bool mayBeEmpty /* = true */, qsizetype maxLength /* = std::numeric_limits<qsizetype>::max() */)
{
    if (json.isNull())
    {
        return {};
    }

    if (!json.isString())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not string: %2")
                                 .arg(path)
                                 .arg(json.toVariant().toString()));
    }

    const auto result = json.toString();

    if (!mayBeEmpty && result.isEmpty())
    {
        throw ParseException(QString("Invalid value of key (%1). Value cannot be empty").arg(path));
    }

    if (result.length() > maxLength)
    {
        throw ParseException(QString("Invalid value of key (%1). Value: %2. The value must be a string no longer than %3 characters")
                                 .arg(path)
                                 .arg(result)
                                 .arg(maxLength));
    }

    return {result};
}

std::optional<bool> Common::JSONReadBool(const QJsonValue &json, const QString &path)
{
    if (json.isNull())
    {
        return {};
    }

    if (!json.isBool())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not bool: %2").arg(path).arg(json.toVariant().toString()));
    }

    return {json.toBool()};
}

std::optional<QDateTime> Common::JSONReadDateTime(const QJsonValue &json, const QString &path, const QString &format)
{
    if (json.isNull())
    {
        return {};
    }

    if (!json.isString())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not Date/Time on format: %2. Value: %3").arg(path).arg(format).arg(json.toVariant().toString()));
    }

    const auto resultStr = json.toString();
    const auto dateTime = QDateTime::fromString(resultStr, format);

    if (!dateTime.isValid())
    {
        throw ParseException(QString("Invalid value of key (%1). The value must be Date/Time on format: %2. Value: %3")
                                 .arg(path)
                                 .arg(format)
                                 .arg(resultStr));
    }

    return {dateTime};
}

QJsonObject Common::JSONReadMapToMap(const QJsonObject& json, const QString& key, const QString& path)
{
    const auto it_value = json.find(key);
    if (it_value == json.end())
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    if (!it_value->isObject())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not map").arg(path));
    }

    return it_value->toObject();
}

QJsonArray Common::JSONReadMapToArray(const QJsonObject& json, const QString& key, const QString& path)
{
    const auto it_value = json.find(key);
    if (it_value == json.end())
    {
        throw ParseException(QString("JSON object does not contain a required key: %1").arg(key));
    }

    if (!it_value->isArray())
    {
        throw ParseException(QString("Invalid value of key (%1). Value is not array").arg(path));
    }

    return it_value->toArray();
}

QJsonObject Common::JSONReadDocumentToMap(const QJsonDocument& json)
{
    if (!json.isObject())
    {
        throw ParseException(QString("JSON document is not JSON object"));
    }

    return json.object();
}

QJsonArray Common::JSONReadDocumentToArray(const QJsonDocument& json)
{
    if (!json.isArray())
    {
        throw ParseException(QString("JSON document is not JSON array"));
    }

    return json.array();
}

QJsonDocument Common::JSONParseToDocument(const QByteArray& json)
{
    QJsonParseError error;
    const auto jsonDoc = QJsonDocument::fromJson(json, &error);

    if (error.error != QJsonParseError::NoError)
    {
        throw ParseException(QString("Error parse JSON document: %1").arg(error.errorString()));
    }

    return jsonDoc;
}

QJsonObject Common::JSONParseToMap(const QByteArray& json)
{
    const auto jsonDoc = JSONParseToDocument(json);

    return JSONReadDocumentToMap(jsonDoc);
}

QJsonArray Common::JSONParseToArray(const QByteArray& json)
{
    const auto jsonDoc = JSONParseToDocument(json);

    return JSONReadDocumentToArray(jsonDoc);
}

QJsonObject Common::JSONReadMap(const QJsonValue &json, const QString &path)
{
    if (json.isArray())
    {
        throw ParseException(QString("Invalid value of key (%1). JSON value is not JSON object").arg(path));
    }

    return json.toObject();
}

QJsonArray Common::JSONReadArray(const QJsonValue &json, const QString &path)
{
    if (!json.isArray())
    {
        throw ParseException(QString("Invalid value of key (%1). JSON value is not JSON array").arg(path));
    }

    return json.toArray();
}
