#ifndef SERIAL_H
#define SERIAL_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

class Serial : public QObject
{
    Q_OBJECT
public:
    explicit Serial(QObject *parent = 0);
    qint8 readNum;
    QString readStr;

signals:
    void readFinished(QString);

public slots:
    void openSerialPort(QString);
    bool scanSerialPort(QStringList *);

private slots:
    void readData();

private:
    QSerialPort *myserial;
};

#endif // SERIAL_H
