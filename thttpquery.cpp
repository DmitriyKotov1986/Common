#include "thttpquery.h"

//Qt
#include <QCoreApplication>

//My
#include "common.h"

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
    , _manager()
    , _url(url)
{
}

THTTPQuery::~THTTPQuery()
{
    delete _manager;
}

void THTTPQuery::send(const QByteArray& data, const QString& url)
{
    if (_manager == nullptr)
    {
        _manager = new QNetworkAccessManager(this);
        _manager->setTransferTimeout(600000);
        QObject::connect(_manager, SIGNAL(finished(QNetworkReply *)),
                         SLOT(replyFinished(QNetworkReply *))); //событие конца обмена данными
    }

    Q_ASSERT(_manager != nullptr);

    //создаем и отправляем запрос
    if (!url.isEmpty())
    {
        _url = url;
    }
    QNetworkRequest request(_url);
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, false);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    request.setHeader(QNetworkRequest::UserAgentHeader, QCoreApplication::applicationName());
    request.setHeader(QNetworkRequest::ContentLengthHeader, QString::number(data.size()));
    request.setTransferTimeout(600000);

    QNetworkReply* resp = _manager->post(request, data);

    writeDebugLogFile("HTTP request:", QString(data));

    if (resp == nullptr)
    {
        emit errorOccurred("Send HTTP request fail");

        return;
    }
}

void THTTPQuery::replyFinished(QNetworkReply *resp)
{
    Q_ASSERT(resp);

    if (resp->error() == QNetworkReply::NoError)
    {
        QByteArray answer = resp->readAll();

        writeDebugLogFile("HTTP answer:", QString(answer));

        emit getAnswer(answer);
    }
    else
    {
        emit errorOccurred("HTTP request fail. Code: " + QString::number(resp->error()) + " Msg: " + resp->errorString());
    }

    resp->deleteLater();
    resp = nullptr;
}




