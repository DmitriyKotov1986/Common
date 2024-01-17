#include "regcheck.h"

//Qt
#include <QCoreApplication>
#include <QProcess>
#include <QCryptographicHash>
#include <QTime>
#include <QRandomGenerator>
#include <QByteArray>
#include <QString>

//My
#include "common.h"

using namespace Common;

//static
const qint64 RegCheck::MAGIC_NUMBER = 1046297243667342302;

const quint64 RegCheck::VERSION = 1u;

const QList<QPair<QString, quint16>> RegCheck::SERVER_LIST =
    {
        {"331.kotiki55.ru", 53923},
        {"srv.ts-azs.com", 53923},
        {"srv.sysat.su", 53923},
        {"127.0.0.1", 53923}
    };

const QByteArray RegCheck::PASSPHRASE("RegService");

//class
RegCheck::RegCheck(const QString& userName, QObject *parent)
    : QTcpSocket{parent}
    , _id(getId())
    , _userName(userName)
{
    _messageString = QString("No Registration Verification Servers available. Please, contact the registration service");

    QRandomGenerator *rg = QRandomGenerator::global();
    _randomNumber = static_cast<qint64>(rg->generate64());


    for (quint8 tryNumber = 1; tryNumber < 6; ++tryNumber)
    {
        quint8 serverNumber = 0;
        for (auto server_it = SERVER_LIST.begin(); server_it != SERVER_LIST.end(); ++server_it)
        {
            ++serverNumber;

            if (state() != QAbstractSocket::UnconnectedState)
            {
                disconnectFromHost();
            }

            connectToHost(server_it->first, server_it->second, QIODevice::ReadWrite, QAbstractSocket::IPv4Protocol);

            if (!waitForConnected(5000))
            {
                QString msg = QString("Error connect to registration server. Try: %1. Server: %2. Error: %3")
                        .arg(tryNumber).arg(serverNumber).arg(errorString());
                qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
                Common::writeLogFile("WAR>", msg);

                continue;
            }

            sendRequest();

            waitForBytesWritten(5000);

            if (!waitForDisconnected(5000))
            {
                QString msg = QString("Timeout get answer from registration server. Try: %1. Server: %2. Error: %3")
                        .arg(tryNumber).arg(serverNumber).arg(errorString());
                qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
                Common::writeLogFile("WAR>", msg);

                continue;
            }

            auto answer = readAll();

            parseAnswer(answer);

            return;
        }
    }
}

RegCheck::~RegCheck()
{
    close();
}

QByteArray RegCheck::runWMIC(const QStringList& keys)
{
    QProcess process(this);
    process.start("wmic.exe" ,keys);
    if (!process.waitForFinished(5000))
    {
        process.kill();

        return QByteArray();
    };

    return process.readAllStandardOutput();
}

QString RegCheck::getId()
{
    auto id = runWMIC(QStringList({"CPU", "GET", "Name,Manufacturer,ProcessorID,SerialNumber"})); //CPU
    id += runWMIC(QStringList({"BASEBOARD", "GET", "Manufacturer,Product,SerialNumber"})); //Motherboard
    id += runWMIC(QStringList({"BIOS", "GET", "Manufacturer,BIOSVersion"})); //BIOS
    id += runWMIC(QStringList({"MEMORYCHIP", "GET", "Manufacturer,PartNumber,SerialNumber"})); //Memory

    QCryptographicHash HASH(QCryptographicHash::Md5);
    HASH.addData(id);

    return HASH.result().toHex();
}

QString RegCheck::parseNext(QByteArray &data, const QString &errMsg)
{
    auto indexOfSeparate = data.indexOf('#');
    if (indexOfSeparate < 1)
    {
        throw QString(errMsg);
    }

    auto res = data.mid(0, indexOfSeparate);
    data.remove(0, indexOfSeparate + 1);

    return res;
}

void RegCheck::sendRequest()
{
    const QString appName = QString("%1: %2").arg(QCoreApplication::applicationName()).arg(QCoreApplication::applicationVersion());
    //[RegService]#[Length]#[User name]#[ID]#[Random number]#[Application name]#[Reg service client version]
    const QString data = QString("%1#%2#%3#%4#%5").arg(_userName).arg(_id).arg(_randomNumber).arg(appName).arg(VERSION).toUtf8();

    const QByteArray sendData = QString("RegService#%1#%2").arg(data.length()).arg(data).toUtf8();

    write(sendData);
}

bool RegCheck::parseAnswer(QByteArray &answer)
{
    try
    {
        //парсим ответ
        if (parseNext(answer, "Incorrect passprase format") != PASSPHRASE)
        {
            throw QString("Incorrect passprase");
        }

        bool ok = false;
        auto size = parseNext(answer, "Incorrect length format").toLongLong(&ok);
        if (!ok)
        {
            throw QString("Incorrect length value");
        }
        if (answer.length() != size)
        {
            throw QString("Incorrect length");
        }

        answer += '#';

        QString res = parseNext(answer, "Incorrect result format");
        if (res == QString("REG"))
        {
            auto answerNumber = parseNext(answer, "Incorrect answer number format").toLongLong(&ok);
            if (!ok)
            {
                throw QString("Incorrect answer number value");
            }

           // qDebug() << answerNumber << MAGIC_NUMBER << (answerNumber ^ MAGIC_NUMBER) << _randomNumber;
            if ((answerNumber ^ MAGIC_NUMBER) != _randomNumber)
            {
                throw QString("Server response validation error");
            }
            else
            {
                _isChecked = true;
            }
        }
        else if (res ==  QString("UNREG"))
        {
            throw QString("Please, contact the registration service");
        }
        else if (res ==  QString("ERROR"))
        {
            auto msg = parseNext(answer, "Incorrect error message format");

            throw QString(msg);
        }

    }
    catch (const QString& errMsg)
    {
        _messageString = errMsg;

        return false;
    }

    return true;
}



