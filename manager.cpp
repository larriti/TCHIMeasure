#include "manager.h"
#include <QDebug>
#include <QFile>
#include "mytype.h"

Manager::Manager(QObject *parent) : QObject(parent)
{
    mSerial = new Serial(this);
    mDatabase = new Database(this);
    connect(mSerial, SIGNAL(readFinished(QString)), mDatabase, SLOT(insertData(QString)));
}

void Manager::Run()
{
    QStringList portNameList;
    QFile file("SensorMeasure.log");

    if(!file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        qDebug("Open log file failed!\t\t%s", qPrintable(DATETIME));
    }
    else
    {
        QTextStream stream(&file);
        stream << QString("System running\t\t%1\n").arg(DATETIME);
        qDebug("System running\t\t%s", qPrintable(DATETIME));

        stream << "Serialport scanning\t\t%1\n" + DATETIME;
        qDebug("Serialport scanning\t\t%s", qPrintable(DATETIME));
        if(mSerial->scanSerialPort(&portNameList))
        {
            mSerial->openSerialPort(portNameList.at(0));
        }
        else
        {
            stream << QString("No device found!\t\t%1\n").arg(DATETIME);
            qDebug("No device found!\t\t%s", qPrintable(DATETIME));
        }
        mDatabase->databaseConnect();
        file.close();
    }
}
