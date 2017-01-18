#include "manager.h"
#include <QDebug>
#include <QThread>
#include <QSettings>
#include <QCoreApplication>
#include "mytype.h"

bool firstboot = true;
float savePara[8][3] = {0};
int goinTimes = 0;

Manager::Manager(QObject *parent) : QObject(parent)
{
    mSerial = new Serial(this);
    mDatabase = new Database(this);
    logFile = new QFile(LOGPATH);
    QSettings setting(QSettings::UserScope, QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    setting.beginGroup("house");
    house_id = setting.value("HouseID", 1).toString();
    terminal_first_id = setting.value("TerminalFirstID", 1).toInt();
    terminal_num = setting.value("TerminalNum", 1).toInt();
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

    // 数据库连接
    qDebug() << "[Database] database connecting...";
    database_connect = mDatabase->databaseConnect();

    // 串口扫描及连接
    qDebug() << "[SerialPort] Serialport scaning...";
    serialport_scan = mSerial->scanSerialPort(&portNameList);
    if(serialport_scan)
    {
        qDebug() << "[SerialPort] Available ports:";
        qDebug() << portNameList;
        serialport_open = mSerial->openSerialPort(portNameList.at(0));
        if(serialport_open)
        {
            qDebug() << tr("[SerialPort] %1 opened").arg(portNameList.at(0));
            mSerial->serialWriteData();
            mSerial->mTimer.start();
        }
        else
        {
            qDebug() << "[SerialPort] serialport open failed";
        }
    }
    else
    {
        qDebug() << "[SerialPort] no available port";
    }

    logFile->open(QIODevice::WriteOnly | QIODevice::Append);
    /* write the info to the log file */
    if(logFile->isOpen())
    {
        qDebug() << "[Log] log file opened";
        QTextStream stream(logFile);
        stream << QString("\n\n[System] running\t\t\t%1\n").arg(DATETIME);
        if(database_connect)
        {
            qDebug() << "[Database] database connected";
            stream << QString("[Database] database connected\t\t%1\n").arg(DATETIME);
        }
        else
        {
            qDebug() << "[Database] database connect failed";
            stream << QString("[Database] database connect failed\t\t%1\n").arg(DATETIME);
        }


        if(serialport_scan)
        {
            if(serialport_open)
            {
                stream << QString("[SerialPort] serialport opened\t\t%1\n").arg(DATETIME);
            }
            else
            {
                stream << QString("[SerialPort] serialport open failed\t\t%1\n").arg(DATETIME);
            }

        }
        else
        {
            stream << QString("[SerialPort] no available port\t\t%1\n").arg(DATETIME);
        }
        logFile->close();
    }
    else
    {
        qDebug() << "[Log] log file open failed";
    }
}

void Manager::readFinished(QByteArray readData)
{
    logFile->open(QIODevice::WriteOnly | QIODevice::Append);
    /* CRC Checknum */
    if(CRCCheck(readData) == true)
    {
        /* 进入次数加一以便于球平均值 */
        goinTimes++;
        if(goinTimes>terminal_num)
            goinTimes = 0;

        bool uploadStatus;
        QStringList truePara, uploadPara, avgPara;

        bool ok;
        int terminal_id = QString(readData.toHex()).mid(0*2,2).toInt(&ok, 16);
        float newTemperature = float(QString(readData.toHex()).mid(3*2,4).toInt(&ok, 16))/10;
        float newHumidity = float(QString(readData.toHex()).mid(5*2,4).toInt(&ok, 16))/10;
        int newCarbon = QString(readData.toHex()).mid(9*2,4).toInt(&ok, 16);

        /* 上传真实数据给管理员看 */
        truePara.append(house_id);
        truePara.append(QString::number(terminal_id));
        truePara.append(QString::number(newTemperature));
        truePara.append(QString::number(newHumidity));
        truePara.append(QString::number(newCarbon));
        /* upload the read datas to the history_data_true database */
        mDatabase->uploadData(truePara, "history_data_true");

        /* 错误数据处理之后上传给用户看 */
        uploadPara.append(house_id);
        uploadPara.append(QString::number(terminal_id));
        /* first boot upload the read datas to the history database */
        if(firstboot == true)
        {
            if( (-10<newTemperature) && (newTemperature<50) )
            {
                uploadPara.append(QString::number(newTemperature));
                savePara[goinTimes-1][0] = newTemperature;
            }
            else
            {
                uploadPara.append(QString::number(0));
                savePara[goinTimes-1][0] = 0;
            }

            if( (0<newHumidity) && (newHumidity<=100) )
            {
                uploadPara.append(QString::number(newHumidity));
                savePara[goinTimes-1][1] = newHumidity;
            }
            else
            {
                uploadPara.append(QString::number(0));
                savePara[goinTimes-1][1] = 0;
            }

            if( (0<newCarbon) && (newCarbon<4000))
            {
                uploadPara.append(QString::number(newCarbon));
                savePara[goinTimes-1][2] = newCarbon;
            }
            else
            {
                uploadPara.append(QString::number(0));
                savePara[goinTimes-1][2] = 0;
            }
            uploadStatus = mDatabase->uploadData(uploadPara, "history_data");
        }
        else
        {
            float lastTemperature = savePara[goinTimes-1][0];
            float lastHumidity = savePara[goinTimes-1][1];
            int lastCarbon = int(savePara[goinTimes-1][2]);
            /* 在-10-50度范围之内且变化不超过10度 */
            if( (-10<newTemperature) && (newTemperature<50) && (qAbs(newTemperature-lastTemperature)<10) )
            {
                uploadPara.append(QString::number(newTemperature));
                savePara[goinTimes-1][0] = newTemperature;
            }
            else
            {
                uploadPara.append(QString::number(lastTemperature));
            }
            /* 在0-100%范围之内且变化不超过20% */
            if( (0<newHumidity) && (newHumidity<=100) && (qAbs(newHumidity-lastHumidity)<20))
            {
                uploadPara.append(QString::number(newHumidity));
                savePara[goinTimes-1][1] = newHumidity;
            }
            else
            {
                uploadPara.append(QString::number(lastHumidity));
            }
            /* 在0-4000ppm范围之内且变化不超过500ppm */
            if( (0<newCarbon) && (newCarbon<4000) && (qAbs(newCarbon-lastCarbon)<500))
            {
                uploadPara.append(QString::number(newCarbon));
                savePara[goinTimes-1][2] = newCarbon;
            }
            else
            {
                uploadPara.append(QString::number(lastCarbon));
            }
            uploadStatus = mDatabase->uploadData(uploadPara, "history_data");
        }

        /* 最后上传平均值到0号终端 */
        if(terminal_id == terminal_first_id+terminal_num-1)
        {
            float allTemperature = 0;
            float allHumidity = 0;
            float allCarbon = 0;
            for(int i=0; i<goinTimes; i++)
            {
                allTemperature += savePara[i][0];
                allHumidity += savePara[i][1];
                allCarbon += savePara[i][2];
            }
            float avgTemperature = allTemperature / goinTimes;
            float avgHumidity = allHumidity / goinTimes;
            int avgCarbon = int(allCarbon / goinTimes);
            avgPara.append(house_id);
            avgPara.append(QString::number(0));
            avgPara.append(QString::number(avgTemperature, 'f', 1));
            avgPara.append(QString::number(avgHumidity, 'f', 1));
            avgPara.append(QString::number(avgCarbon));
            mDatabase->uploadData(avgPara, "history_data");

            firstboot = false;
            goinTimes = 0;
        }

        /* Parameter compare and upload the alarm information */
        QStringList comparePara = uploadPara;
        comparePara.removeFirst();
        comparePara.removeFirst();
        emit this->paraCompare(comparePara, uploadPara.at(1));

        /* write the info to the log file */
        if(logFile->isOpen())
        {
            QTextStream stream(logFile);

            qDebug() << QString("%1 %2 %3 %4 %5").arg(uploadPara.at(0)).arg(uploadPara.at(1))
                        .arg(uploadPara.at(2)).arg(uploadPara.at(3)).arg(uploadPara.at(4));
            stream << QString("%1 %2 %3 %4 %5\t\t").arg(uploadPara.at(0)).arg(uploadPara.at(1))
                      .arg(uploadPara.at(2)).arg(uploadPara.at(3)).arg(uploadPara.at(4));
            /* upload serialdata to database */
            if(uploadStatus)
            {
                qDebug() << tr("%1 uploaded").arg(DATETIME);
                stream << QString("%1\tuploaded\n").arg(DATETIME);
            }
            else
            {
                qDebug() << tr("%1 upload failed").arg(DATETIME);
                stream << QString("%1\tupload failed\n").arg(DATETIME);
            }
            logFile->close();
        }
    }
    else
    {
        if(logFile->isOpen())
        {
            QTextStream stream(logFile);
            qDebug() << "[Transmission] data transmission error";
            stream << "[Transmission] data transmission error\n";
            logFile->close();
        }
    }
}

void Manager::serialError(QString errorstring)
{
    logFile->open(QIODevice::WriteOnly | QIODevice::Append);
    if(logFile->isOpen())
    {
        QTextStream stream(logFile);
        qDebug() << QString("[SerialPort] %1").arg(errorstring);
        stream << QString("[SerialPort] %1\t\t%2\n").arg(errorstring).arg(DATETIME);
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
    QSettings setting(QSettings::UserScope, QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    setting.beginGroup("manager");
    deleteDate = setting.value("DeleteTime", "1994-07-18").toDate();
    deleteInterval = setting.value("DeleteInterval", 15).toUInt();
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
