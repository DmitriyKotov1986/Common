
//QT
#include <QCoreApplication>
#include <QAuthenticator>
#include <QSslConfiguration>
#include <QMutex>
#include <QMutexLocker>

#include "Common/httpsslquery.h"

using namespace Common;

static const qsizetype MAX_MANAGER_COUNT = 2000;

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

#ifdef QT_DEBUG
    qDebug() << QString("HTTPS REQUEST (%1) TO %2 %3%4%5")
                    .arg(id)
                    .arg(requestTypeToString(type))
                    .arg(url.toString())
                    .arg(curHeaders.empty() ? "" : QString(". Headers: %1").arg(headersToString(curHeaders)))
                    .arg(data.isEmpty() ? "" : QString(". Data: %1").arg(data));
#endif

    auto manager = getManager(id);

    //Если количество одновренменных превышено - то гененрируем сигнал с ошибкой
    if (!manager)
    {
        QTimer::singleShot(1, this,
            [this, id]()
            {
            const auto msg = QString("HTTP request fail. HTTPSSLClient: Too many requests sent at the same time");

            emit errorOccurred(QNetworkReply::NetworkError::OperationCanceledError, 0, msg, id);
            });

        return id;
    }

    //создаем и отправляем запрос
    QNetworkRequest request(url);
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());

    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, false);

    for (auto header_it = curHeaders.begin(); header_it != curHeaders.end(); ++header_it)
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

    freeManager(id);

#ifdef QT_DEBUG
    qDebug() << QString("HTTPS ANSWER (%1) FROM %2: %3").arg(id).arg(resp->url().toString()).arg(answer);
#endif

    if (resp->error() == QNetworkReply::NoError)
    {
        emit getAnswer(answer, id);
    }
    else
    {
        const auto serverCode = resp->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto msg = QString("HTTP request fail. Code: %1. Server code: %2. Messasge: %3.%4 Answer: %5")
                             .arg(QString::number(resp->error()))
                             .arg(serverCode)
                             .arg(resp->errorString())
                             .arg(resp->manager()->proxy().hostName().isEmpty() ? "" : QString(" Proxy: %1:%2.").arg(resp->manager()->proxy().hostName()).arg(resp->manager()->proxy().port()))
                             .arg(answer);

        emit errorOccurred(resp->error(), serverCode, msg, id);
    }

    resp->deleteLater(); 
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

        QObject::connect(_managerPool.get(), SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
                         SLOT(sslErrors(QNetworkReply*, const QList<QSslError>&)));
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

NetworkAccessManagerPool::~NetworkAccessManagerPool()
{
    delete _clearTimer;
}

QNetworkAccessManager* NetworkAccessManagerPool::getManager(quint64 id)
{
    Q_ASSERT(!_busyManager.contains(id));

    //Find free manager
    std::multimap<quint64, PManager> _freeManager;

    for (const auto& manager: _poolManager)
    {
        if (manager.second->isFree)
        {
            _freeManager.emplace(manager.second->queryCount, manager.first);
        }
    }

    //слишком много одновременных запросов
    if (_freeManager.empty() && _poolManager.size() > MAX_MANAGER_COUNT)
    {
        return nullptr;
    }

    PManager useManager = _freeManager.empty() ? addManager() : _freeManager.begin()->second;

    auto& useManagerInfo = _poolManager.at(useManager);
    useManagerInfo->isFree = false;
    useManagerInfo->lastUse = QDateTime::currentDateTime();
    ++useManagerInfo->queryCount;

    _busyManager.emplace(id, useManager);

    if (!_clearTimer)
    {
        _clearTimer = new QTimer();

        QObject::connect(_clearTimer, SIGNAL(timeout()), SLOT(clearUnusedManager()));

        _clearTimer->start(60 * 1000); //1мин
    }

    return useManager.get();
}

void NetworkAccessManagerPool::freeManager(quint64 id)
{
    const auto it_busyManager = _busyManager.find(id);

    Q_ASSERT(it_busyManager != _busyManager.end());

    const auto p_manager = it_busyManager->second;

    auto& useManagerInfo = _poolManager.at(p_manager);
    useManagerInfo->isFree = true;

    _busyManager.erase(it_busyManager);
}

NetworkAccessManagerPool::PManager NetworkAccessManagerPool::addManager()
{
    PManager result;
    if (_proxyList.isEmpty())
    {
        auto manager = std::make_shared<QNetworkAccessManager>();
        auto managerInfo = std::make_unique<ManagerInfo>();

        addManager(manager, std::move(managerInfo));

        result = manager;
    }
    else
    {
        for (const auto& proxy: _proxyList)
        {
            auto manager = std::make_shared<QNetworkAccessManager>();
            manager->setProxy(proxy);

            auto managerInfo = std::make_unique<ManagerInfo>();

            addManager(manager, std::move(managerInfo));

            result = manager;
        }
    }

    return result;
}

void NetworkAccessManagerPool::addManager(PManager& manager, PManagerInfo&& managerInfo)
{
    QObject::connect(manager.get(), SIGNAL(finished(QNetworkReply*)),
                     SLOT(replyFinishedManager(QNetworkReply*)));
    QObject::connect(manager.get(), SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
                     SLOT(authenticationRequiredManager(QNetworkReply*, QAuthenticator*)));
    QObject::connect(manager.get(), SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)),
                     SLOT(proxyAuthenticationRequiredManager(const QNetworkProxy&, QAuthenticator*)));
    QObject::connect(manager.get(), SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
                     SLOT(sslErrorsManager(QNetworkReply*, const QList<QSslError>&)));

    manager->setTransferTimeout(10 * 1000);

    _poolManager.emplace(manager, std::move(managerInfo));

#ifdef QT_DEBUG
    qDebug() << QString("Added new NetworkAccessManager. Total managers: %1").arg(_poolManager.size());
#endif
}

void NetworkAccessManagerPool::clearUnusedManager()
{
    //Создаем список неиспользуемых менеджеров
    const auto oldPoolSize = _poolManager.size();
    const auto curDateTime = QDateTime::currentDateTime();
    std::list<PManager> eraseList;
    for (const auto& [p_manager, managerInfo]: _poolManager)
    {
        if (managerInfo->isFree && managerInfo->lastUse.secsTo(curDateTime) > 60)
        {
            eraseList.push_back(p_manager);
        }
    }

    for (const auto& p_manager: eraseList)
    {
        _poolManager.erase(p_manager);
    }

    if (_poolManager.empty())
    {
        _clearTimer->deleteLater();
        _clearTimer = nullptr;
    }

#ifdef QT_DEBUG
    if (oldPoolSize != _poolManager.size())
    {
        qDebug() << QString("Erase %1 unused NetworkAccessManager. Total managers: %2").arg(oldPoolSize - _poolManager.size()).arg(_poolManager.size());
    }
#endif

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

void NetworkAccessManagerPool::sslErrorsManager(QNetworkReply *reply, const QList<QSslError> &errors)
{
    emit sslErrors(reply, errors);
}


