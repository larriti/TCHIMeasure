#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlRecord>
#include <QtSql/QSqlError>
#include <QTimer>

class Database : public QObject
{
    Q_OBJECT
public:
    explicit Database(QObject *parent = 0);
    ~Database();

signals:

public slots:
    bool databaseConnect();
    bool uploadData(QStringList);
    void paraCompare(QStringList, QString);
    void uploadtAlarm(QString);

private slots:


private:
    QSqlDatabase myDatabase;
    QString house_id;
};

#endif // DATABASE_H
