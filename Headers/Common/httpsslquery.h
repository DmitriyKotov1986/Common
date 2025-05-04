#pragma once

//STL
#include <memory>
#include <unordered_map>

//QT
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkProxy>
#include <QString>
#include <QVector>
#include <QUrl>
#include <QByteArray>
#include <QDateTime>
#include <QTimer>

//My
#include "Common/tdbloger.h"

namespace Common
{

class NetworkAccessManagerPool; ///< Пул сетевых менеджеров. Будет объявлен позднее

class HTTPSSLQuery final
    : public QObject
{
    Q_OBJECT

public:
    using Headers = QHash<QByteArray, QByteArray>; ///< Список заголовоков пакета

    /*!
        Типы запросов
     */
    enum class RequestType: quint8
    {
        POST = 1,   ///< POST запрос
        GET = 2     ///< GET запрос
    };

    using ProxyList = QList<QNetworkProxy>; ///< Список прокси

public:
    /*!
        Возвращает уникальный ИД запроса. ИД будет уникален даже при использовании нескольких экземплятов HTTPSSLQuery. ИД запроса != 0. Этот метод потокобезопасен
        @return - ИД запроса
     */
    static quint64 getId();

private:
    /*!
        Преобразвет заголовки запроса в строку
        @param headers -заголовки запроса
        @return строка
    */
    static QString headersToString(const Headers& headers);

    /*!
        Преобразвет текст запроса в строку
        @param type -тип запроса
        @return строковое представление типа запроса
     */
    static QString requestTypeToString(RequestType type);

public:
    /*!
        Конструктор. Планируется использовать только этот конструтор
        @param proxyList - список прокси. Пустое список = прокси не используется
        @param parent - указатель на родительский класс
    */
    explicit HTTPSSLQuery(const ProxyList& proxyList = ProxyList{}, QObject* parent = nullptr);

    /*!
        Деструктор
    */
    ~HTTPSSLQuery() override;

    /*!
        Устанавливает имя пользователя и пароль для автоизации на сервере, если она требется. Этот метод должен быть вызван до выполнения первого запроса.
        @param user - имя пользователя
        @param password - пароль
    */
    void setUserPassword(const QString& user, const QString& password);

    void setHeaders(const Headers& headers);

    /*!
        Запускает отправку запроса. в результате обработки запроса будет сгенерирован сигнал getAnswer(...) в случае успешной обработки запроса
            сервером. либо errorOccurred(...) в случае ошибки. Таймаут выполнения бапроса вместе м передачей данных - 30 сек
        @param url - адрес запроса. Должен быть валидным
        @param type - тип запроса
        @param headers - заголовки запроса. Значение заголовков Use-Agent ( = название программы) и C
            ontent-Length( = размеру  data) будут установлены автоматически
        @param data - даные передаваемые в запросе
        @return - уникаьный ИД запроса
    */
    quint64 send(const QUrl& url,
              Common::HTTPSSLQuery::RequestType type,
              const Headers& headers = Headers{},
              const QByteArray& data = QByteArray()); //

signals:
    /*!
        Получен положительный ответ от сервера
        @param answer данные ответа
        @param id - ИД запроса
    */
    void getAnswer(const QByteArray& answer, quint64 id);

    /*!
        Ошибка обработки запроса
        @param code - код ошибки
        @param serverCode - код ответа сервера (или 0 если ответ не получен)
        @param msg - сообщение об ошибке
        @param id - ИД запроса
    */
    void errorOccurred(QNetworkReply::NetworkError code, quint64 serverCode, const QString& msg, quint64 id);

    /*!
        Дополнительное сообщение логеру
        @param category - категория сообщения
        @param msg - текст сообщения
        @param id -ИД запроса
    */
    void sendLogMsg(Common::TDBLoger::MSG_CODE category, const QString& msg, quint64 id);

private slots:
    /*!
        Обработка запроса завершена
        @param resp указател ьна ответ. Не может равняться nullptr
    */
    void replyFinished(QNetworkReply* resp);

    /*!
        Поступил запрос авторизации от сервера
        @param reply - указатель на запрос
        @param authenticator - указатель на класс с параметрами авторизации
    */
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);

    /*!
        Поступил запрос авторизации от прокси
        @param proxy - прокси
        @param authenticator - указатель на класс с параметрами авторизации
    */
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);

#ifndef QT_NO_SSL
    /*!
        Ошибка SSL
        @param reply -указатель на ответ на запрос
        @param errors - список ошибок
    */
    void sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
#endif

private:
    // Удаляем неиспользуеме конструторы
    Q_DISABLE_COPY_MOVE(HTTPSSLQuery);

    /*!
        Возвращеат указатель на менеджер из пула и создает пул при первом использовании
        @param id - ИД запроса
        @return  - указатель на менджер
    */
    QNetworkAccessManager* getManager(quint64 id);

    /*!
        Освобождает менеджер после обработки запроса
        @param id - ИД запроса
    */
    void freeManager(quint64 id);

private:
    const HTTPSSLQuery::ProxyList _proxyList;                       ///< Список прокси ля отправки запросов
    std::unique_ptr<Common::NetworkAccessManagerPool> _managerPool; ///< Пул мменеджер обработки соединений

    QString _user;      ///< Имя пользователя
    QString _password;  ///< Пароль

    Headers _headers;
};

///////////////////////////////////////////////////////////////////////////////
///     class NetworkAccessManagerPool - пул менеджеров сетевых подключений.
///         Вспомогательный класс. Обесечивает балансировку количества запросов между прокси
///

class NetworkAccessManagerPool final
    : public QObject
{
    Q_OBJECT

public:
    /*!
        Конструтор. Предполагается использование только этот конструтор
        @param proxyList - список используемых прокси. Если список пустой - прокси не используются
    */
    NetworkAccessManagerPool(const HTTPSSLQuery::ProxyList& proxyList);

    /*!
        Деструктор
    */
    ~NetworkAccessManagerPool() override = default;

    /*!
        Возвращает указатель на текущий менеджер или nullptr если превышено количество допустимых менеджеров
        (слишком много запросов)
        @param id - ИД запроса. Этот же ИД следует указать при вызове freeManager(...)
        return - указатель на текущий менеджер или nullptr
    */
    QNetworkAccessManager* getManager(quint64 id);

    /*!
        Возвращает менеджер в пулл
        @param id - ИД запроса
    */
    void freeManager(quint64 id);

signals:
    void replyFinished(QNetworkReply* resp);
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);

#ifndef QT_NO_SSL
    void sslErrors(QNetworkReply *reply, const QList<QSslError> &errors);
#endif

private slots:
    void replyFinishedManager(QNetworkReply* resp);
    void authenticationRequiredManager(QNetworkReply *reply, QAuthenticator *authenticator);
    void proxyAuthenticationRequiredManager(const QNetworkProxy &proxy, QAuthenticator *authenticator);

#ifndef QT_NO_SSL
    void sslErrorsManager(QNetworkReply *reply, const QList<QSslError> &errors);
#endif

private:
    using PManager = std::unique_ptr<QNetworkAccessManager>;///< Указатель на менеджер

private:
    //  Удаляем неиспользуемые конструторы
    NetworkAccessManagerPool() = delete;
    Q_DISABLE_COPY_MOVE(NetworkAccessManagerPool);

    /*!
        Создает новый менеджер или несколько менеджеров для каждого из доступных прокси
    */
    void addManager();

    /*!
        Добавляет новый менеджер в пул и связывает его сигалы/слоты
        @param manager - указатель на новый менеджер
        @param managerInfo - указатель на сведения о менеджере
    */
    void addManager(PManager&& manager);

private:
    const HTTPSSLQuery::ProxyList _proxyList;                   ///< Список прокси

    std::queue<PManager> _poolManager;                          ///< Пул менеджеров
    std::unordered_map<quint64, PManager> _busyManager;         ///< Список свободных менеджеров. Ключ - ИД запроса, занчение - указатель на менеджер обслуживающий этот запрос

};

}

Q_DECLARE_METATYPE(Common::HTTPSSLQuery::Headers)
Q_DECLARE_METATYPE(Common::HTTPSSLQuery::RequestType)

