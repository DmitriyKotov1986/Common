#pragma once

//QT
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkProxy>
#include <QString>
#include <QVector>
#include <QUrl>
#include <QByteArray>

//My
#include "Common/tdbloger.h"

namespace Common
{

class HTTPSSLQuery final
    : public QObject
{
    Q_OBJECT

public:
    using Headers = QHash<QByteArray, QByteArray>;

    enum class RequestType: quint8
    {
        POST,
        GET
    };

    using ProxyList = QList<QNetworkProxy>;

public:
    static quint64 getId();

private:
    static QString headersToString(const Headers& headers);

public:
    explicit HTTPSSLQuery(const ProxyList& proxyList = ProxyList{}, QObject* parent = nullptr);
    ~HTTPSSLQuery();

    void setUserPassword(const QString& user, const QString& password);

    quint64 send(const QUrl& url,
              Common::HTTPSSLQuery::RequestType type,
              const QByteArray& data,
              const Common::HTTPSSLQuery::Headers& headers); //запускает отправку запроса

signals:
    void getAnswer(const QByteArray& answer, quint64 id);
    void errorOccurred(QNetworkReply::NetworkError code, quint64 serverCode, const QString& msg, quint64 id);
    void sendLogMsg(Common::TDBLoger::MSG_CODE category, const QString& msg, quint64 id);

private slots:
    void replyFinished(QNetworkReply* resp); //конец приема ответа
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
    void sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    Q_DISABLE_COPY_MOVE(HTTPSSLQuery)

    QNetworkAccessManager* getManager();

private:
    QVector<QNetworkAccessManager*> _managerPool; //менеджер обработки соединений
    int _currentManager = 0;

    QString _user;
    QString _password;
};

}

Q_DECLARE_METATYPE(Common::HTTPSSLQuery::Headers)
Q_DECLARE_METATYPE(Common::HTTPSSLQuery::RequestType)
