#ifndef REGCHECK_H
#define REGCHECK_H

#include <QtNetwork/QTcpSocket>
#include <QString>
#include <QHash>

namespace Common
{

class RegCheck : public QTcpSocket
{
private:
    static const qint64 MAGIC_NUMBER;
    static const quint64 VERSION;
    static const QList<QPair<QString, quint16>> SERVER_LIST;
    static const QByteArray PASSPHRASE;

public:
    explicit RegCheck(const QString& userName, QObject *parent = nullptr);
    ~RegCheck();

    bool isChecket() const { return _isChecked; }
    const QString& id() const {return _id; }

    const QString& messageString() const { return _messageString; };

private:
    QString getId();
    QString parseNext(QByteArray &data, const QString &errMsg);
    void sendRequest();
    QByteArray runWMIC(const QStringList& keys);

private:
    bool _isChecked = false;
    QString _messageString;
    const QString _id;
    const QString _userName;
    qint64 _randomNumber = 0;
    bool parseAnswer(QByteArray& answer); //тут нам нужна именно не константная ссылка, т.к. в процессе парсинша мы будем изменять данные и далее они нам не понадобятся

}; //class RegCheck

} //namespace Common

#endif // REGCHECK_H
