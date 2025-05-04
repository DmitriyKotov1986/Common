#pragma once

//QT
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QString>
#include <QTimer>
#include <QHash>

namespace Common
{

///////////////////////////////////////////////////////////////////////////////
///     The THTTPQuery class - класс для отправки простых POST запросов
///
class THTTPQuery final
    : public QObject
{
    Q_OBJECT

public:
    static THTTPQuery* HTTPQuery(const QString& url = "", QObject* parent = nullptr);
    static void deleteTHTTPQuery();

public:
    ~THTTPQuery() override;

private:
    THTTPQuery() = delete;
    Q_DISABLE_COPY_MOVE(THTTPQuery)

    explicit THTTPQuery(const QString& url, QObject* parent = nullptr);

public slots:
    void send(const QByteArray& data); //запускает отправку запроса

signals:
    void getAnswer(const QByteArray& answer);
    void errorOccurred(const QString& msg);

private slots:
    void replyFinished(QNetworkReply* resp); //конец приема ответа

private:
    //service
    QNetworkAccessManager* _manager = nullptr; //менеджер обработки соединений

    //Data
    const QString _url; //адресс на который отправляется запрос

};

} //namespace Common
