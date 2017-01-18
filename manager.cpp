#include "manager.h"
#include <QDebug>
#include <QThread>
#include <QSettings>
#include "mytype.h"

Manager::Manager(QObject *parent) : QObject(parent)
{
    mSerial = new Serial(this);
    mDatabase = new Database(this);
    logFile = new QFile(LOGPATH);
    QSettings setting(CONFIGPATH, QSettings::IniFormat);
    setting.beginGroup("house");
    house_id = setting.value("HouseID").toString();
    setting.endGroup();

    connect(mSerial, SIGNAL(readFinished(QByteArray)), this, SLOT(readFinished(QByteArray)));
    connect(this, SIGNAL(paraCompare(QStringList, QString)), mDatabase, SLOT(paraCompare(QStringList, QString)));
    connect(mSerial, SIGNAL(serialError(QString)), this, SLOT(serialError(QString)));
    connect(&portScanTimer, SIGNAL(timeout()), this, SLOT(serialScan()));
    portScanTimer.setInterval(1000);
    connect(&deleteFileTimer, SIGNAL(timeout()), this, SLOT(deleteLogFile()));
    deleteFileTimer.start(30000);
}

Manager::~Manager()
{
    if(logFile->isOpen())
        logFile->close();
    delete logFile;
    delete mSerial;
    delete mDatabase;
    delete this;
}

void Manager::Run()
{
    QStringList portNameList;
    bool database_connect=false, serialport_scan=false, serialport_open=false;

    database_connect = mDatabase->databaseConnect();
    serialport_scan = mSerial->scanSerialPort(&portNameList);
    if(serialport_scan)
    {
        serialport_open = mSerial->openSerialPort(portNameList.at(0));
        if(serialport_open)
        {
            mSerial->serialWriteData();
            mSerial->mTimer.start();
        }
    }

    logFile->open(QIODevice::WriteOnly | QIODevice::Append);
    /* write the info to the log file */
    if(logFile->isOpen())
    {
        QTextStream stream(logFile);
        stream << QString("\n\nSystem running\t\t\t%1\n").arg(DATETIME);
        if(database_connect)
        {
            stream << QString("Database connected\t\t%1\n").arg(DATETIME);
        }
        else
        {
            stream << QString("[DatabaseError]: Database connect failed\t\t%1\n").arg(DATETIME);
        }


        if(serialport_scan)
        {
            if(serialport_open)
            {
                stream << QString("Serialport opened\t\t%1\n").arg(DATETIME);
            }
            else
            {
                stream << QString("[SerialError]: Serialport open failed\t\t%1\n").arg(DATETIME);
            }

        }
        else
        {
            stream << QString("[SerialError]: No device found!\t\t%1\n").arg(DATETIME);
        }
        logFile->close();
    }
}

void Manager::readFinished(QByteArray readData)
{
    logFile->open(QIODevice::WriteOnly | QIODevice::Append);
    /* CRC Checknum */
    if(CRCCheck(readData) == true)
    {
        bool ok;
        /* Add the readdata parameter split to the qstringlist*/
        QStringList receivePara;
        receivePara.append(house_id);
        receivePara.append(QString::number(QString(readData.toHex()).mid(0*2,2).toInt(&ok, 16)));
        receivePara.append(QString::number(float(QString(readData.toHex()).mid(3*2,4).toInt(&ok, 16))/10));
        receivePara.append(QString::number(float(QString(readData.toHex()).mid(5*2,4).toInt(&ok, 16))/10));
        receivePara.append(QString::number(QString(readData.toHex()).mid(9*2,4).toInt(&ok, 16)));

        /* upload the readdata to the database */
        ok = mDatabase->uploadData(receivePara);

        /* Parameter compare and upload the alarm information */
        QStringList comparePara = receivePara;
        comparePara.removeFirst();
        comparePara.removeFirst();
        emit this->paraCompare(comparePara, receivePara.at(1));

        /* write the info to the log file */
        if(logFile->isOpen())
        {
            QTextStream stream(logFile);

            stream << QString("%1 %2 %3 %4 %5\t\t")
                      .arg(receivePara.at(0))
                      .arg(receivePara.at(1))
                      .arg(receivePara.at(2))
                      .arg(receivePara.at(3))
                      .arg(receivePara.at(4));
            /* upload serialdata to database */
            if(ok)
            {
                stream << QString("%1\t\tUploaded\n").arg(DATETIME);
            }
            else
            {
                stream << QString("%1\t\tUpload failed\n").arg(DATETIME);
            }
            logFile->close();
        }
    }
    else
    {
        if(logFile->isOpen())
        {
            QTextStream stream(logFile);
            stream << "Data transmission error\n";
            logFile->close();
        }
    }
//    mSerial->readywrite = true;
}

void Manager::serialError(QString errorstring)
{
    logFile->open(QIODevice::WriteOnly | QIODevice::Append);
    if(logFile->isOpen())
    {
        QTextStream stream(logFile);
        stream << QString("[SerialError]: %1\t\t%2\n").arg(errorstring).arg(DATETIME);
        mSerial->mTimer.stop();
        portScanTimer.start();
        logFile->close();
    }
}

void Manager::serialScan()
{
    QStringList portNameList;
    if(mSerial->scanSerialPort(&portNameList))
    {
        if(mSerial->openSerialPort(portNameList.at(0)))
        {
            portScanTimer.stop();
            mSerial->mTimer.start();
        }
    }
}

void Manager::deleteLogFile()
{
    QSettings setting(CONFIGPATH, QSettings::IniFormat);
    setting.beginGroup("manager");
    deleteDate = setting.value("DeleteTime").toDate();
    deleteInterval = setting.value("DeleteInterval",15).toUInt();
    setting.endGroup();
    if( deleteDate.daysTo(QDate::currentDate()) >= deleteInterval )
    {
        if(logFile->isOpen())
        {
            logFile->close();
        }
        logFile->remove();
        setting.beginGroup("manager");
        setting.setValue("DeleteTime", QDate::currentDate().toString("yyyy-MM-dd"));
        setting.endGroup();
    }
}

bool Manager::CRCCheck(QByteArray buf)
{
    bool ok;
    uint ReceiveCRC = buf.toHex().mid((buf.length()-1)*2,2).toUInt(&ok, 16)*256 +
                    buf.toHex().mid((buf.length()-2)*2,2).toInt(&ok, 16);
    buf.remove(buf.length()-2, 2);
    if(ReceiveCRC == CRC16(buf))
        return true;
    else
        return false;
}

uint Manager::CRC16(QByteArray buf)
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
