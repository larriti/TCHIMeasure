#include "serial.h"
#include <QDebug>
#include <QDateTime>
#include <QString>
#include <QStringList>
#include "mytype.h"

Serial::Serial(QObject *parent) : QObject(parent)
{
    myserial = new QSerialPort(this);
    connect(myserial, SIGNAL(readyRead()), this, SLOT(readData()));
}

void Serial::openSerialPort(QString portName)
{
    myserial->setPortName(portName);
    myserial->setBaudRate(QSerialPort::Baud9600);
    myserial->setDataBits(QSerialPort::Data8);
    myserial->setParity(QSerialPort::NoParity);
    myserial->setStopBits(QSerialPort::OneStop);
    if(myserial->open(QIODevice::ReadWrite))
    {
        qDebug("Serialport opened\t\t%s", qPrintable(DATETIME));
    }
    else
    {
        qDebug("[Error] Serialport open failed\t\t%s", qPrintable(DATETIME));
    }
}

void Serial::readData()
{
    QByteArray readByte;

    readByte = myserial->readAll();
    readStr += QString(readByte);
    readNum += readByte.length();
    if(readNum == 14)
    {
        emit readFinished(readStr);

        readNum = 0;
        readStr.clear();
    }
}

bool Serial::scanSerialPort(QStringList *portNameList)
{
    QStringList tempList;
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        tempList.append(info.portName());
    }

    if(tempList.count() != 0)
    {
        *portNameList = tempList.filter("ttyUSB");
        return true;
    }
    else
    {
        return false;
    }
}
