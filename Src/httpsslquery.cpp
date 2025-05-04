
//QT
#include <QCoreApplication>
#include <QAuthenticator>
#include <QSslConfiguration>
#include <QSslError>
#include <QMutex>
#include <QMutexLocker>

#include "Common/httpsslquery.h"

using namespace Common;

static const qsizetype MAX_LOG_LENGTH = 1024 * 10; //10KB
static quint64 TRANSFER_TIMEOUT = 30 * 1000; //30c

///////////////////////////////////////////////////////////////////////////////
///     class HTTPSSLQuery
///
quint64 HTTPSSLQuery::getId()
{
    static quint64 id = 0;
    static QMutex getIDMutex;

    QMutexLocker<QMutex> locker(&getIDMutex);

    return ++id;
}

QString HTTPSSLQuery::headersToString(const Headers& headers)
{
    QString result;
    QTextStream ss(&result);

    bool isFirst = true;
    for (auto headers_it = headers.begin(); headers_it != headers.end(); ++headers_it)
    {
        if (!isFirst)
        {
            ss << "; ";
        }
        isFirst = false;

        ss << headers_it.key() << ":" << headers_it.value();
    }

    return result;
}

QString HTTPSSLQuery::requestTypeToString(RequestType type)
{
    switch (type)
    {
    case RequestType::POST: return "POST";
    case RequestType::GET: return "GET";
    default:
        Q_ASSERT(false);
    }

    return "UNDEFINE";
}

//class
HTTPSSLQuery::HTTPSSLQuery(const ProxyList& proxyList /* = t = ProxyList{} */, QObject* parent /* = nullptr */)
    : QObject{parent}
    , _proxyList(proxyList)
{
    qRegisterMetaType<Common::HTTPSSLQuery::Headers>("Headers");
    qRegisterMetaType<Common::HTTPSSLQuery::RequestType>("RequestType");
    qRegisterMetaType<QNetworkReply::NetworkError>("NetworkError");
}

HTTPSSLQuery::~HTTPSSLQuery()
{
}

void HTTPSSLQuery::setUserPassword(const QString &user, const QString &password)
{
    _user = user;
    _password = password;
}

void HTTPSSLQuery::setHeaders(const Headers &headers)
{
    _headers = headers;
}

quint64 HTTPSSLQuery::send(const QUrl& url, RequestType type, const Headers& headers /* = Headers{} */, const QByteArray& data /* = QByteArray() */)
{
    Q_ASSERT(url.isValid());

    const auto id = getId();

    Headers curHeaders(_headers);
    curHeaders.insert(headers);

    auto manager = getManager(id);

    //Если количество одновренменных превышено - то гененрируем сигнал с ошибкой
    if (!manager)
    {
        QTimer::singleShot(1, this,
            [this, id]()
            {
            const auto msg = QString("HTTP request fail. HTTPSSLClient: Too many requests sent at the same time. ");

            emit errorOccurred(QNetworkReply::NetworkError::OperationCanceledError, 0, msg, id);
            });

        return id;
    }

#ifdef QT_DEBUG
    const auto dataForLog = data.size() > MAX_LOG_LENGTH ? data.first(MAX_LOG_LENGTH) : data;

    qDebug() << QString("HTTPS REQUEST (%1) TO %2 %3%4%5%6")
                    .arg(id)
                    .arg(requestTypeToString(type))
                    .arg(manager->proxy().hostName().isEmpty() ? "" : QString("(Proxy: %1:%2) ").arg(manager->proxy().hostName()).arg(manager->proxy().port()))
                    .arg(url.toString())
                    .arg(curHeaders.empty() ? "" : QString(". Headers: %1").arg(headersToString(curHeaders)))
                    .arg(data.isEmpty() ? "" : QString(". Data(%1 Bytes): %2").arg(data.size()).arg(data));
#endif

    //создаем и отправляем запрос
    QNetworkRequest request(url);

#ifndef QT_NO_SSL
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());
#endif

    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, false);

    curHeaders.emplace("User-Agent", QCoreApplication::applicationName().toUtf8());
    if (data.size() != 0)
    {
        curHeaders.emplace("Content-Length", QString::number(data.size()).toUtf8());
    }

    for (auto header_it = curHeaders.begin(); header_it != curHeaders.end(); ++header_it)
    {
        request.setRawHeader(header_it.key(), header_it.value());
    }

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

    Q_CHECK_PTR(resp);

    resp->setObjectName(QString::number(id));

    return id;
}

void HTTPSSLQuery::replyFinished(QNetworkReply *resp)
{
    Q_CHECK_PTR(resp);

    QByteArray answer;
    if (resp->isOpen())
    {
        answer = resp->readAll();
    }

    bool ok= false;
    const auto id = resp->objectName().toULongLong(&ok);
    Q_ASSERT(ok);

    const auto answerForLog = QString(answer.size() > MAX_LOG_LENGTH ? answer.first(MAX_LOG_LENGTH) : answer);
#ifdef QT_DEBUG
    qDebug() << QString("HTTPS ANSWER (%1) FROM %2: (%3 Bytes) %4").arg(id).arg(resp->url().toString()).arg(answer.size()).arg(answerForLog);
#endif

    if (resp->error() == QNetworkReply::NoError)
    {
        emit getAnswer(answer, id);
    }
    else
    {
        const auto serverCode = resp->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto msg = QString("HTTP request fail. Code: %1. Server code: %2. Messasge: %5.%4 Answer: %3") //используем неправильный порядок аргументов т.к. resp->errorString() может содержать символ %
                             .arg(QString::number(resp->error()))
                             .arg(serverCode)
                             .arg(answerForLog)
                             .arg(resp->manager()->proxy().hostName().isEmpty() ? "" : QString(" Proxy: %1:%2.").arg(resp->manager()->proxy().hostName()).arg(resp->manager()->proxy().port()))
                             .arg(resp->errorString());

        emit errorOccurred(resp->error(), serverCode, msg, id);
    }

    QTimer::singleShot(0, this,
        [this, id, resp]()
        {
            delete resp;
            freeManager(id);
        });
}

void HTTPSSLQuery::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_CHECK_PTR(authenticator);
    Q_CHECK_PTR(reply);

#ifdef QT_DEBUG
    qDebug() << QString("Get authentication required from: %1:%2. Send user: %3, password: %4")
                    .arg(reply->request().url().host())
                    .arg(reply->request().url().port())
                    .arg(_user)
                    .arg(_password);
#endif

    authenticator->setUser(_user);
    authenticator->setPassword(_password);
    authenticator->setOption("realm", authenticator->realm());
}

void HTTPSSLQuery::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
    Q_CHECK_PTR(authenticator);

#ifdef QT_DEBUG
    qDebug() << QString("Get proxy authentication required from: %1:%2. Seed user: %3, password: %4")
                    .arg(proxy.hostName())
                    .arg(proxy.port())
                    .arg(_user)
                    .arg(_password);
#endif

    authenticator->setUser(proxy.user());
    authenticator->setPassword(proxy.password());
    authenticator->setOption("realm", authenticator->realm());
}

#ifndef QT_NO_SSL
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

    emit sendLogMsg(Common::TDBLoger::MSG_CODE::WARNING_CODE, QString("SSL Error: %1").arg(errorsString), reply->objectName().toULongLong());
}
#endif

QNetworkAccessManager *HTTPSSLQuery::getManager(quint64 id)
{
    if (!_managerPool)
    {
        _managerPool = std::make_unique<NetworkAccessManagerPool>(_proxyList);

        QObject::connect(_managerPool.get(), SIGNAL(replyFinished(QNetworkReply*)),
                         SLOT(replyFinished(QNetworkReply*)));

        QObject::connect(_managerPool.get(), SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
                         SLOT(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));

        QObject::connect(_managerPool.get(), SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
                         SLOT(authenticationRequired(QNetworkReply*, QAuthenticator*)));

#ifndef QT_NO_SSL
        QObject::connect(_managerPool.get(), SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
                         SLOT(sslErrors(QNetworkReply*, const QList<QSslError>&)));
#endif
    }

    return _managerPool->getManager(id);
}

void HTTPSSLQuery::freeManager(quint64 id)
{
    Q_CHECK_PTR(_managerPool);

    _managerPool->freeManager(id);
}

///////////////////////////////////////////////////////////////////////////////
///     class NetworkAccessManagerPool
///
NetworkAccessManagerPool::NetworkAccessManagerPool(const HTTPSSLQuery::ProxyList &proxyList)
    : _proxyList(proxyList)
{
}

QNetworkAccessManager* NetworkAccessManagerPool::getManager(quint64 id)
{
    Q_ASSERT(!_busyManager.contains(id));

    if (_busyManager.size() > 1000)
    {
        return nullptr;
    }

    if (_poolManager.size() == 0)
    {
        addManager();
    }

    Q_ASSERT(!_poolManager.empty());

    auto useManager = std::move(_poolManager.front());
    const auto p_useManager = useManager.get();

    _poolManager.pop();

    _busyManager.emplace(id, std::move(useManager));

    return p_useManager;
}

void NetworkAccessManagerPool::freeManager(quint64 id)
{
    const auto it_busyManager = _busyManager.find(id);

    Q_ASSERT(it_busyManager != _busyManager.end());

    auto manager = std::move(it_busyManager->second);
    manager->clearConnectionCache();
    manager->clearAccessCache();

    _busyManager.erase(it_busyManager);

    _poolManager.push(std::move(manager));
}

void NetworkAccessManagerPool::addManager()
{
    if (_proxyList.isEmpty())
    {
        auto manager = std::make_unique<QNetworkAccessManager>();

        addManager(std::move(manager));
    }
    else
    {
        for (const auto& proxy: _proxyList)
        {
            auto manager = std::make_unique<QNetworkAccessManager>();
            manager->setProxy(proxy);

            addManager(std::move(manager));
        }
    }
}

void NetworkAccessManagerPool::addManager(PManager&& manager)
{
    QObject::connect(manager.get(), SIGNAL(finished(QNetworkReply*)),
                     SLOT(replyFinishedManager(QNetworkReply*)));
    QObject::connect(manager.get(), SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
                     SLOT(authenticationRequiredManager(QNetworkReply*, QAuthenticator*)));
    QObject::connect(manager.get(), SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
                     SLOT(proxyAuthenticationRequiredManager(const QNetworkProxy&, QAuthenticator*)));

#ifndef QT_NO_SSL
    QObject::connect(manager.get(), SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
                     SLOT(sslErrorsManager(QNetworkReply*, const QList<QSslError>&)));
#endif

    manager->setTransferTimeout(TRANSFER_TIMEOUT);  //

#ifdef QT_DEBUG
    qDebug() << QString("Added new NetworkAccessManager%1")
                    .arg(manager->proxy().hostName().isEmpty() ? "" : QString(". Use proxy: %1:%2").arg(manager->proxy().hostName()).arg(manager->proxy().port()));
#endif

    _poolManager.push(std::move(manager));
}

void NetworkAccessManagerPool::replyFinishedManager(QNetworkReply* resp)
{
    emit replyFinished(resp);
}

void NetworkAccessManagerPool::authenticationRequiredManager(QNetworkReply *reply, QAuthenticator *authenticator)
{
    emit authenticationRequired(reply, authenticator);
}

void NetworkAccessManagerPool::proxyAuthenticationRequiredManager(const QNetworkProxy &proxy, QAuthenticator *authenticator)
{
    emit proxyAuthenticationRequired(proxy, authenticator);
}

#ifndef QT_NO_SSL
void NetworkAccessManagerPool::sslErrorsManager(QNetworkReply *reply, const QList<QSslError> &errors)
{
    emit sslErrors(reply, errors);
}
#endif

