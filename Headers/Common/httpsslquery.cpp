//QT
#include <QCoreApplication>
#include <QAuthenticator>
#include <QSslConfiguration>
#include <QMutex>
#include <QMutexLocker>

#include "Common/httpsslquery.h"

using namespace Common;

static quint32 MANAGER_COUNT = 10;

static quint64 getID()
{
    static quint64 id = 0;
    static QMutex getIDMutex;

    QMutexLocker<QMutex> locker(&getIDMutex);

    return ++id;
}

//class
HTTPSSLQuery::HTTPSSLQuery(const ProxyList& proxyList, QObject* parent /* = nullptr */)
    : QObject{parent}
{
    qRegisterMetaType<Common::HTTPSSLQuery::Headers>("Headers");
    qRegisterMetaType<Common::HTTPSSLQuery::RequestType>("RequestType");
    qRegisterMetaType<QNetworkReply::NetworkError>("NetworkError");

    qsizetype proxyIndex = 0;
    for (quint32 i = 0; i < MANAGER_COUNT; ++i)
    {
        auto manager = new QNetworkAccessManager();
        if (!proxyList.empty())
        {
            manager->setProxy(proxyList.at(proxyIndex));

            ++proxyIndex;
            if (proxyIndex >= proxyList.size())
            {
                proxyIndex = 0;
            }
        }

        QObject::connect(manager, SIGNAL(finished(QNetworkReply *)),
                         SLOT(replyFinished(QNetworkReply *))); //событие конца обмена данными

        QObject::connect(manager, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
                         SLOT(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));

        QObject::connect(manager, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
                         SLOT(authenticationRequired(QNetworkReply*, QAuthenticator*)));

        QObject::connect(manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
                         SLOT(sslErrors(QNetworkReply*, const QList<QSslError>&)));

        _managerPool.push_back(manager);
    }
}

HTTPSSLQuery::~HTTPSSLQuery()
{
    for (auto manager: _managerPool)
    {
        manager->disconnect();

        delete manager;
    }
}

void HTTPSSLQuery::setUserPassword(const QString &user, const QString &password)
{
    _user = user;
    _password = password;
}

quint64 HTTPSSLQuery::send(const QUrl& url, RequestType type, const QByteArray& data, const Headers& headers)
{
    auto manager = getManager();

    //создаем и отправляем запрос
    QNetworkRequest request(url);
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());

    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, false);
    for (auto header_it = headers.begin(); header_it != headers.end(); ++header_it)
    {
        request.setRawHeader(header_it.key(), header_it.value());
    }
    request.setHeader(QNetworkRequest::UserAgentHeader, QCoreApplication::applicationName());
    request.setHeader(QNetworkRequest::ContentLengthHeader, QString::number(data.size()));

    QNetworkReply* resp = nullptr;
    switch (type)
    {
    case RequestType::POST:
    {
        resp = manager->post(request, data);

        break;
    }
    case RequestType::GET:
    {
        resp = manager->get(request);

        break;
    }
    default:
        Q_ASSERT(false);
    }

    const auto id = getID();
    resp->setObjectName(QString::number(id));

    return id;
}

void HTTPSSLQuery::replyFinished(QNetworkReply *resp)
{
    Q_CHECK_PTR(resp);

    if (resp->error() == QNetworkReply::NoError)
    {
        QByteArray answer = resp->readAll();

        emit getAnswer(answer, resp->objectName().toULongLong());
    }
    else
    {
        QByteArray answer;
        if (resp->isOpen())
        {
            answer = resp->readAll();
        }

        const auto serverCode = resp->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto msg = QString("HTTP request fail. Code: %1. Server code: %2. Messasge: %3.%4 Answer: %5")
                             .arg(QString::number(resp->error()))
                             .arg(serverCode)
                             .arg(resp->errorString())
                             .arg(resp->manager()->proxy().hostName().isEmpty() ? "" : QString(" Proxy: %1:%2.").arg(resp->manager()->proxy().hostName()).arg(resp->manager()->proxy().port()))
                             .arg(answer);

        emit errorOccurred(resp->error(), serverCode, msg, resp->objectName().toULongLong());
    }

    resp->deleteLater();
}

void HTTPSSLQuery::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_CHECK_PTR(authenticator);
    Q_CHECK_PTR(reply);

    authenticator->setUser(_user);
    authenticator->setPassword(_password);
    authenticator->setOption("realm", authenticator->realm());
}

void HTTPSSLQuery::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
    Q_CHECK_PTR(authenticator);

    authenticator->setUser(proxy.user());
    authenticator->setPassword(proxy.password());
    authenticator->setOption("realm", authenticator->realm());
}

void HTTPSSLQuery::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_CHECK_PTR(reply);

    reply->ignoreSslErrors();

    QString errorsString;
    for (const auto& error: errors)
    {
        if (!errorsString.isEmpty())
        {
            errorsString += "; ";
        }

        errorsString += error.errorString();
    }

    qWarning() << "SSL Error: " << errors.first().errorString();
}

QNetworkAccessManager *HTTPSSLQuery::getManager()
{
    ++_currentManager;

    if (_currentManager >= _managerPool.size())
    {
        _currentManager = 0;
    }

    return _managerPool.at(_currentManager);
}



