#ifndef SERIAL_H
#define SERIAL_H

#include <QObject>
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QTimer>

class Serial : public QObject
{
    Q_OBJECT
public:
    explicit Serial(QObject *parent = 0);
    ~Serial();
    QByteArray readByteArray;
    QTimer mTimer;

signals:

    void readFinished(QByteArray);
    void canWriteData();
    void serialError(QString);

public slots:

    bool openSerialPort(QString);
    bool scanSerialPort(QStringList *);
    void serialWriteData();

private slots:

    void serialReadData();
    void serialErrorHandle(QSerialPort::SerialPortError);
    void serialTimeout();
    void timerTimeout();
    uint CRC16(QByteArray);

private:

    QSerialPort *mySerial;
    quint16 house_id;
    quint8 terminal_num;
    quint8 terminal_first_id;
    quint64 seconds;
    quint64 writeinterval;
};

#endif // SERIAL_H
