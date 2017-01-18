#include "serial.h"
#include <QDebug>
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QSettings>
#include <QCoreApplication>
#include <QThread>
#include "mytype.h"
#include "manager.h"

quint8 id;

Serial::Serial(QObject *parent) : QObject(parent)
{
    mySerial = new QSerialPort(this);
    QSettings setting(QSettings::UserScope, QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());

    setting.beginGroup("serial");
    mySerial->setBaudRate(setting.value("BaudRate", "9600").toLongLong());
    mySerial->setDataBits(QSerialPort::DataBits(setting.value("DataBits", "8").toInt()));
    mySerial->setParity(QSerialPort::Parity(setting.value("Parity", "0").toInt()));
    mySerial->setStopBits(QSerialPort::StopBits(setting.value("StopBits", "1").toInt()));
    writeinterval = setting.value("WriteInterval", 5).toLongLong();
    setting.endGroup();

    setting.beginGroup("house");
    house_id = setting.value("HouseID", 1).toUInt();
    terminal_first_id = setting.value("TerminalFirstID", 1).toInt();
    id=terminal_first_id;
    terminal_num = setting.value("TerminalNum", 1).toUInt();
    setting.endGroup();

    connect(mySerial, SIGNAL(readyRead()), this, SLOT(serialReadData()));
    connect(mySerial, SIGNAL(error(QSerialPort::SerialPortError)), this, SLOT(serialErrorHandle(QSerialPort::SerialPortError)));
    connect(&mTimer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    connect(this, SIGNAL(canWriteData()), this, SLOT(serialTimeout()));
    this->seconds = 0;
    mTimer.setInterval(1000);
//    readywrite = true;
}

Serial::~Serial()
{
    delete mySerial;
    delete this;
}

void Serial::timerTimeout()
{
    seconds ++;
    if(seconds>=writeinterval)
    {
        seconds = 0;
        emit canWriteData();
    }
}

bool Serial::scanSerialPort(QStringList *portNameList)
{
    QStringList tempList;
    foreach(const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
    {
        tempList.append(info.portName());
    }
    *portNameList = tempList.filter("ttyUSB");
    if(portNameList->count() != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Serial::openSerialPort(QString portName)
{
    mySerial->setPortName(portName);
    if(mySerial->open(QIODevice::ReadWrite))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void Serial::serialWriteData()
{
    if(mySerial->isOpen() && mySerial->isWritable())
    {
        static const char tempchar[] = {3, 0, 42, 0, 4};
        QByteArray tempByte(tempchar, sizeof(tempchar));

        tempByte.prepend(id);
        uint crc = CRC16(tempByte);
        tempByte.append(crc%256);
        tempByte.append(crc/256);
//        qDebug() << "Send: " << tempByte.toHex();
        mySerial->write(tempByte);

        id++;
        if (id == terminal_first_id + terminal_num)
        {
            id = terminal_first_id;
        }
    }
}

void Serial::serialReadData()
{
    if(mySerial->bytesAvailable()==13)
    {
        readByteArray = mySerial->readAll();
        bool ok;
        int temp = QString(readByteArray.toHex()).mid(0,2).toInt(&ok, 16);
        if((terminal_first_id <= temp) && (temp < terminal_first_id + terminal_num))
        {
            emit readFinished(readByteArray);
        }
        else
        {
            // Clear the received data
            mySerial->close();
            mySerial->open(QIODevice::ReadWrite);
        }
    }
    else
    if(mySerial->bytesAvailable()>13)
    {
        // Read the all received data
         mySerial->readAll();
    }

}


void Serial::serialTimeout()
{
    this->serialWriteData();
}

void Serial::serialErrorHandle(QSerialPort::SerialPortError serialerror)
{
    if(serialerror != QSerialPort::NoError)
    {
        if(mySerial->isOpen())
        {
            mySerial->close();
        }
        emit serialError(mySerial->errorString());
    }
}

uint Serial::CRC16(QByteArray buf)
{
    uint crc = 0xFFFF;

    for (int pos = 0; pos < buf.length(); pos++)
    {
        crc ^= (uint)buf[pos];                  // XOR byte into least sig. byte of crc

        for (int i = 8; i != 0; i--)
        {
            /* Loop over each bit */
            if ((crc & 0x0001) != 0)
            {
                /* If the LSB is set */
                crc >>= 1;                      // Shift right and XOR 0xA001
                crc ^= 0xA001;
            }
            else                                // Else LSB is not set
                crc >>= 1;                      // Just shift right
        }
    }
    /* Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes) */
    return crc;
}
