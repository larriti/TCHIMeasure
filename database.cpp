#include "database.h"
#include <QDebug>
#include <QDateTime>
#include <QSettings>
#include <QStringList>
#include "mytype.h"

Database::Database(QObject *parent) : QObject(parent)
{
    myDatabase = QSqlDatabase::addDatabase("QMYSQL", "First");
    myDatabase.setConnectOptions("MYSQL_OPT_CONNECT_TIMEOUT=5");
    myDatabase.setConnectOptions("MYSQL_OPT_RECONNECT=TRUE");
    QSettings setting(CONFIGPATH, QSettings::IniFormat);
    setting.beginGroup("database");
    myDatabase.setDatabaseName(setting.value("DatabaseName","lk").toString());
    myDatabase.setHostName(setting.value("HostName", "localhost").toString());
    myDatabase.setUserName(setting.value("UserName", "0").toString());
    myDatabase.setPassword(setting.value("Password", "0").toString());
    setting.endGroup();

    setting.beginGroup("house");
    house_id = setting.value("HouseID").toString();
    setting.endGroup();
}

Database::~Database()
{
    delete this;
}

bool Database::databaseConnect()
{
    if(myDatabase.open())
    {
        myDatabase.close();
        return true;
    }

    return false;
}

bool Database::uploadData(QStringList serialData)
{
    if(myDatabase.open())
    {
        QSqlQuery query(myDatabase);
        query.prepare("INSERT INTO history_data(HOUSEID, TERMINALID, UPDATETIME, TEMPERATURE, HUMIDITY, CARBON) VALUES (?, ?, ?, ?, ?, ?)");

        query.addBindValue(serialData.at(0));
        query.addBindValue(serialData.at(1));
        query.addBindValue(DATETIME);
        query.addBindValue(serialData.at(2));
        query.addBindValue(serialData.at(3));
        query.addBindValue(serialData.at(4));
        bool ok = query.exec();
        if(ok)
        {
            return true;
        }
        else
        {
            return false;
        }
        myDatabase.close();
    }

    return false;
}

void Database::paraCompare(QStringList alarm_para, QString terminal_id)
{
    if(myDatabase.open())
    {
        bool ok;
        QSqlQuery query(myDatabase);
        query.prepare("SELECT DOWNTEMPERATURE,TEMPERATURE,DOWNHUMIDITY,HUMIDITY,DOWNCARBON,CARBON FROM alarm_para WHERE HOUSEID=? AND TERMINALID=? ORDER BY ID DESC LIMIT 1");
        query.addBindValue(house_id);
        query.addBindValue(terminal_id);
        ok = query.exec();

        if(ok)
        {
            if(query.size())
            {
                query.next();
                QString comparePara;
                QString settingPara;

                comparePara = alarm_para.at(0);
                settingPara = query.value(0).toString();
                if(comparePara.toFloat(&ok) < settingPara.toFloat(&ok))
                {
                    this->uploadtAlarm(QString("%1号棚%2号终端温度低于%3")
                                       .arg(house_id)
                                       .arg(terminal_id)
                                       .arg(settingPara)
                                       );
                }
                settingPara = query.value(1).toString();
                if(comparePara.toFloat(&ok) > settingPara.toFloat(&ok))
                {
                    this->uploadtAlarm(QString("%1号棚%2号终端温度高于%3")
                                       .arg(house_id)
                                       .arg(terminal_id)
                                       .arg(settingPara)
                                       );
                }


                comparePara = alarm_para.at(1);
                settingPara = query.value(2).toString();
                if(comparePara.toFloat(&ok) < settingPara.toFloat(&ok))
                {
                    this->uploadtAlarm(QString("%1号棚%2号终端湿度低于%3")
                                       .arg(house_id)
                                       .arg(terminal_id)
                                       .arg(settingPara)
                                       );
                }
                settingPara = query.value(3).toString();
                if(comparePara.toFloat(&ok) > settingPara.toFloat(&ok))
                {
                    this->uploadtAlarm(QString("%1号棚%2号终端湿度高于%3")
                                       .arg(house_id)
                                       .arg(terminal_id)
                                       .arg(settingPara)
                                       );
                }

                comparePara = alarm_para.at(2);
                settingPara = query.value(4).toString();
                if(comparePara.toFloat(&ok) < settingPara.toFloat(&ok))
                {
                    this->uploadtAlarm(QString("%1号棚%2号终端CO2浓度低于%3")
                                       .arg(house_id)
                                       .arg(terminal_id)
                                       .arg(settingPara)
                                       );
                }
                settingPara = query.value(5).toString();
                if(comparePara.toFloat(&ok) > settingPara.toFloat(&ok))
                {
                    this->uploadtAlarm(QString("%1号棚%2号终端CO2浓度高于%3")
                                       .arg(house_id)
                                       .arg(terminal_id)
                                       .arg(settingPara)
                                       );
                }
            }
        }
        myDatabase.close();
    }
}

void Database::uploadtAlarm(QString alarm_info)
{
    if(myDatabase.open())
    {
        QSqlQuery query(myDatabase);
        query.prepare("INSERT INTO alarm(ALERTTIME, ALERTINFO) VALUES (?, ?)");
        query.addBindValue(DATETIME);
        query.addBindValue(alarm_info);
        query.exec();
        myDatabase.close();
    }
}
