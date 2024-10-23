#pragma once

#include <QtNetwork/QTcpSocket>
#include <QString>
#include <QHash>

namespace Common
{

class RegCheck final
{
public:
    explicit RegCheck(const QString& userName);
    ~RegCheck();

    bool isChecket() const { return _isChecked; }
    const QString id() const { return _id; }

    const QString& messageString() const { return _messageString; };

private:
    RegCheck() = delete;
    Q_DISABLE_COPY_MOVE(RegCheck)

    static QByteArray getIdData();
    static QString getHash(const QByteArray& data);
    static QByteArray getRegFileId(const QByteArray& idData);
    static QByteArray runWMIC(const QStringList& keys);

    QString parseNext(QByteArray &data, const QString &errMsg);
    QByteArray sendRequest();

private:
    bool _isChecked = false;

    QString _messageString;

    const QByteArray _idData; //Data
    const QString _id;        //MD5 HASH

    const QString _userName;

    qint64 _randomNumber = 0;

    bool parseAnswer(QByteArray& answer); //тут нам нужна именно не константная ссылка, т.к. в процессе парсинга мы будем изменять данные и далее они нам не понадобятся

}; //class RegCheck

} //namespace Common
