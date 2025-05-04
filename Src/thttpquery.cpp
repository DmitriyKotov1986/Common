//Qt
#include <QCoreApplication>

//My
#include "Common/common.h"

#include "Common/thttpquery.h"

using namespace Common;

//static
static THTTPQuery* HTTPQueryPtr = nullptr;

THTTPQuery* THTTPQuery::HTTPQuery(const QString& url, QObject* parent)
{
    if (HTTPQueryPtr == nullptr)
    {
        HTTPQueryPtr = new THTTPQuery(url, parent);
    }

    return HTTPQueryPtr;
}

void THTTPQuery::deleteTHTTPQuery()
{
    Q_CHECK_PTR(HTTPQueryPtr);

    if (HTTPQueryPtr != nullptr)
    {
        delete HTTPQueryPtr;

        HTTPQueryPtr = nullptr;
    }
}

//class
THTTPQuery::THTTPQuery(const QString& url, QObject* parent)
    : QObject(parent)
    , _url(url)
{
    Q_ASSERT(!url.isEmpty());
}

THTTPQuery::~THTTPQuery()
{
    delete _manager;
}

void THTTPQuery::send(const QByteArray& data)
{
    if (_manager == nullptr)
    {
        _manager = new QNetworkAccessManager(this);

        QObject::connect(_manager, SIGNAL(finished(QNetworkReply *)),
                         SLOT(replyFinished(QNetworkReply *))); //событие конца обмена данными
    }

    Q_ASSERT(_manager != nullptr);

    //создаем и отправляем запрос
    QNetworkRequest request(_url);
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, false);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    request.setHeader(QNetworkRequest::UserAgentHeader, QCoreApplication::applicationName());
    request.setHeader(QNetworkRequest::ContentLengthHeader, QString::number(data.size()));

    QNetworkReply* resp = _manager->post(request, data);

    writeDebugLogFile("HTTP request:", QString(data));

    if (!resp)
    {
        emit errorOccurred("Send HTTP request fail");

        return;
    }
}

void THTTPQuery::replyFinished(QNetworkReply *resp)
{
    Q_ASSERT(resp);
    QByteArray answer;
    if (resp->isOpen())
    {
        answer = resp->readAll();
    }

    if (resp->error() == QNetworkReply::NoError)
    {
        writeDebugLogFile("HTTP answer:", QString(answer));

        emit getAnswer(answer);
    }
    else
    {
        const auto serverCode = resp->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto msg = QString("HTTP request fail. Code: %1. Server code: %2. Messasge: %3. Answer: %4")
                             .arg(QString::number(resp->error()))
                             .arg(serverCode)
                             .arg(resp->errorString())
                             .arg(answer);

        emit errorOccurred(msg);
    }

    resp->deleteLater();
}



