//Qt
#include <QCoreApplication>
#include <QProcess>
#include <QCryptographicHash>
#include <QTime>
#include <QRandomGenerator>
#include <QByteArray>
#include <QString>

//My
#include "Common/common.h"

#include "Common/regcheck.h"

using namespace Common;

//static
static const qint64 MAGIC_NUMBER = 1046297243667342302;
static const quint64 VERSION = 1u;
static const QString REG_CHECK_FILE_NAME = "System32/SystemMonitor.reg";
static const QList<QPair<QString, quint16>> SERVER_LIST =
    {
        {"331.kotiki55.ru", 53923},
        {"srv.ts-azs.com", 53923},
        {"srv.sysat.su", 53923},
        {"127.0.0.1", 53923}
    };
static const QByteArray PASSPHRASE("RegService");



//Вспомогательный класс исключения парсинга ответа сервера
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

//class RegCheck
RegCheck::RegCheck(const QString& userName)
    : _idData(getIdData())
    , _id(getHash(_idData))
    , _userName(userName)
{
    _messageString = QString("No Registration Verification Servers available. Please, contact the registration service");

    for (quint8 tryNumber = 1; tryNumber < 6; ++tryNumber)
    {
        quint8 serverNumber = 0;
        for (auto server_it = SERVER_LIST.begin(); server_it != SERVER_LIST.end(); ++server_it)
        {
            ++serverNumber;

            QTcpSocket socket;
            socket.connectToHost(server_it->first, server_it->second, QIODevice::ReadWrite, QAbstractSocket::IPv4Protocol);

            if (!socket.waitForConnected(30000))
            {
                QString msg = QString("Error connect to registration server. Try: %1. Server: %2. Error: %3")
                        .arg(tryNumber).arg(serverNumber).arg(socket.errorString());
                qWarning() << msg;
                Common::writeLogFile("WAR", msg);

                continue;
            }

            socket.write(sendRequest());

            socket.waitForBytesWritten(30000);

            if (!socket.waitForDisconnected(30000))
            {
                QString msg = QString("Timeout get answer from registration server. Try: %1. Server: %2. Error: %3")
                        .arg(tryNumber).arg(serverNumber).arg(socket.errorString());
                qWarning() << msg;
                Common::writeLogFile("WAR", msg);

                continue;
            }

            auto answer = socket.readAll();

            parseAnswer(answer);

            return;
        }
    }
}

RegCheck::~RegCheck()
{
}

QByteArray RegCheck::runWMIC(const QStringList& keys)
{
    QProcess process;
    process.start("wmic.exe" ,keys);
    if (!process.waitForFinished(30000))
    {
        process.kill();

        return QByteArray();
    };

    return process.readAllStandardOutput();
}

QByteArray RegCheck::getIdData()
{
    auto idData = runWMIC(QStringList({"CPU", "GET", "Name,Manufacturer,ProcessorID,SerialNumber"})); //CPU
    idData += runWMIC(QStringList({"BASEBOARD", "GET", "Manufacturer,Product,SerialNumber"})); //Motherboard
    idData += runWMIC(QStringList({"BIOS", "GET", "Manufacturer,BIOSVersion"})); //BIOS
    idData += runWMIC(QStringList({"MEMORYCHIP", "GET", "Manufacturer,PartNumber,SerialNumber"})); //Memory
    idData += runWMIC(QStringList({"DISKDRIVE", "WHERE", "MediaType!='Removable media'", "GET", "Name,Manufacturer,Model,SerialNumber"})); //disk
    idData += runWMIC(QStringList({"OS", "GET", "InstallDate"})); //disk
    idData += getRegFileId(idData);

//    qDebug() << idData;

    return idData;
}

QString RegCheck::getHash(const QByteArray &data)
{
    QCryptographicHash HASH(QCryptographicHash::Md5);
    HASH.addData(data);

    return HASH.result().toHex();
}

QByteArray RegCheck::getRegFileId(const QByteArray& idData)
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    QByteArray result;
    QFileInfo regFileInfo(QString("%1/%2").arg(env.value("windir")).arg(REG_CHECK_FILE_NAME));
    if (regFileInfo.exists())
    {
        QFile regFile(regFileInfo.absoluteFilePath());
        if (!regFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text))
        {
            qWarning() << QString("Cannot open registration file. Error: %1").arg(fileErrorToString(regFile.error()));

            return result;
        }
        result = regFile.readLine();

        regFile.close();
    }
    else
    {
        QFile regFile(regFileInfo.absoluteFilePath());
        if (!regFile.open(QIODeviceBase::WriteOnly | QIODeviceBase::Text))
        {
            qWarning() << QString("Cannot create registration file. Error: %1").arg(fileErrorToString(regFile.error()));

            return result;
        }

        quint64 randomNumber = QRandomGenerator::global()->generate();
        result = QString::number(randomNumber).toUtf8();
        regFile.write(result);
        regFile.write("\r\n");
        regFile.write(idData.toBase64());

        regFile.close();
    }

    return result;
}

QString RegCheck::parseNext(QByteArray &data, const QString &errMsg)
{
    auto indexOfSeparate = data.indexOf('#');
    if (indexOfSeparate < 1)
    {
        throw ParseException(errMsg);
    }

    auto res = data.mid(0, indexOfSeparate);
    data.remove(0, indexOfSeparate + 1);

    return res;
}

QByteArray RegCheck::sendRequest()
{
    _randomNumber = QRandomGenerator::global()->generate();

    const QString appName = QString("%1: %2").arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion());
    //[RegService]#[Length]#[User name]#[ID]#[Random number]#[Application name]#[Reg service client version]
    const QString data = QString("%1#%2#%3#%4#%5").arg(_userName).arg(_id).arg(_randomNumber).arg(appName).arg(VERSION).toUtf8();

    return QString("RegService#%1#%2").arg(data.length()).arg(data).toUtf8();
}

bool RegCheck::parseAnswer(QByteArray &answer)
{
    try
    {
        //парсим ответ
        if (parseNext(answer, "Incorrect passprase format") != PASSPHRASE)
        {
            throw ParseException("Incorrect passprase");
        }

        bool ok = false;
        auto size = parseNext(answer, "Incorrect length format").toLongLong(&ok);
        if (!ok)
        {
            throw ParseException("Incorrect length value");
        }
        if (answer.length() != size)
        {
            throw ParseException("Incorrect length");
        }

        answer += '#';

        QString res = parseNext(answer, "Incorrect result format");
        if (res == QString("REG"))
        {
            auto answerNumber = parseNext(answer, "Incorrect answer number format").toLongLong(&ok);
            if (!ok)
            {
                throw ParseException("Incorrect answer number value");
            }

           // qDebug() << answerNumber << MAGIC_NUMBER << (answerNumber ^ MAGIC_NUMBER) << _randomNumber;
            if ((answerNumber ^ MAGIC_NUMBER) != _randomNumber)
            {
                throw ParseException("Server response validation error");
            }
            else
            {
                _isChecked = true;
            }
        }
        else if (res ==  QString("UNREG"))
        {
            throw ParseException(QString("Please, contact the registration service. Data: %1").arg(_idData.toBase64()));
        }
        else if (res ==  QString("ERROR"))
        {
            auto msg = parseNext(answer, "Incorrect error message format");

            throw ParseException(msg);
        }

    }
    catch (const ParseException& errMsg)
    {
        _messageString = errMsg.what();

        return false;
    }

    return true;
}



