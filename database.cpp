#include "database.h"
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include "mytype.h"

Database::Database(QObject *parent) : QObject(parent)
{
    mydatabase = QSqlDatabase::addDatabase("QMYSQL", "First");
}

void Database::databaseConnect()
{
    mydatabase.setDatabaseName("dbSensor");
    mydatabase.setHostName("172.18.0.116");
    mydatabase.setUserName("dgdz");
    mydatabase.setPassword("dgdz405");
    if(mydatabase.open())
    {
        qDebug("Database connected\t\t%s", qPrintable(DATETIME));
    }
    else
    {
        qDebug("Database connect failed\t\t%s", qPrintable(DATETIME));
        qDebug("[Error]: %s", qPrintable(mydatabase.lastError().text()));
    }
}

void Database::insertData(QString insertStr)
{
    if(mydatabase.isOpen())
    {
        QSqlQuery query(mydatabase);

        query.prepare("INSERT INTO tableSensor(datetime, temperature, humidity, CO2, illumination) VALUES (?, ?, ?, ?, ?)");
        query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss"));
        query.addBindValue(insertStr.left(4));
        query.addBindValue(insertStr.mid(4,3));
        query.addBindValue(insertStr.mid(7,3));
        query.addBindValue(insertStr.right(4));

        bool ok = query.exec();
        if(ok)
        {
            qDebug("Upload data success\t\t%s", qPrintable(DATETIME));
        }
        else
        {
            qDebug("Upload data failed\t\t%s", qPrintable(DATETIME));
            qDebug("[Error]: ", qPrintable(query.lastError().text()));
        }
    }
}
