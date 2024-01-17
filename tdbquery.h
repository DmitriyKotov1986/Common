#ifndef TDBQUERY_H
#define TDBQUERY_H

//Qt
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVariant>

//My
#include "common.h"

namespace Common
{

class TDBQuery : public QObject
{
    Q_OBJECT

public:
    using TRow = QVector<QVariant>;
    using TResultRecords = QVector<TRow>;

public:
    explicit TDBQuery(const DBConnectionInfo& dbConnectionInfo, const QString& queryText, bool selectMode = false, QObject *parent = nullptr);
    ~TDBQuery();

public slots:
    void runQuery();

signals:
    void selectResult(const TDBQuery::TResultRecords& result);
    void errorOccurred(const QString& Msg);
    void finished();

private:
    void exitWithError(const QString& msg);

    int connectionNumber();

private:
    const DBConnectionInfo _dbConnectionInfo;
    const bool _selectMode = false;
    const QString _queryText;

    int _currentConnetionNumber;
};

} // namespace Common

Q_DECLARE_METATYPE(Common::TDBQuery::TResultRecords);

#endif // TDBQUERY_H
