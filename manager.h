#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include <QDateTime>
#include <QFile>
#include <QTimer>
#include "serial.h"
#include "database.h"

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject *parent = 0);
    ~Manager();

signals:

    void paraCompare(QStringList, QString);


public slots:

    void Run();
    void readFinished(QByteArray);

private slots:

    uint CRC16(QByteArray);
    bool CRCCheck(QByteArray);
    void serialError(QString);
    void serialScan();
    void deleteLogFile();

private:

    Serial *mSerial;
    Database *mDatabase;
    QFile   *logFile;
    QFile   *configFile;
    QString serialData;
    QString house_id;
    QTimer portScanTimer;
    QDate   deleteDate;
    QTimer  deleteFileTimer;
    qint16 deleteInterval;
    quint8 terminal_num;
    quint8 terminal_first_id;
};

#endif // MANAGER_H
