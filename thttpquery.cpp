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
    Q_ASSERT(!url.isEmpty());
}

THTTPQuery::~THTTPQuery()
{
    if (_manager != nullptr)
    {
        watchDocTimeout(); // закрываем все соединения

        _manager->deleteLater();
    }
}

void THTTPQuery::send(const QByteArray& data)
{
    if (_manager == nullptr)
    {
        _manager = new QNetworkAccessManager(this);
        _manager->setTransferTimeout(30000);
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
    request.setTransferTimeout(30000);

    QNetworkReply* resp = _manager->post(request, data);

    writeDebugLogFile("HTTP request:", QString(data));

    if (resp != nullptr)
    {
        QTimer* watchDocTimer = new QTimer();
        watchDocTimer->setSingleShot(true);
        QObject::connect(watchDocTimer, SIGNAL(timeout()), SLOT(watchDocTimeout()));
        _watchDocs.insert(resp, watchDocTimer);
        watchDocTimer->start(60000);
    }
    else
    {
        emit errorOccurred("Send HTTP request fail");

        return;
    }
}

void THTTPQuery::replyFinished(QNetworkReply *resp)
{
    Q_ASSERT(resp);
    Q_ASSERT(_watchDocs.contains(resp));

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
    _watchDocs[resp]->stop();
    _watchDocs[resp]->deleteLater();
    _watchDocs.remove(resp);

    resp->deleteLater();
    resp = nullptr;
}

void THTTPQuery::watchDocTimeout()
{
    for (auto const& resp : _watchDocs.keys())
    {
        resp->abort();
    }
}



