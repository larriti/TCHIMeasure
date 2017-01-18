#ifndef MANAGER_H
#define MANAGER_H

#include <QObject>
#include <QDateTime>
#include "serial.h"
#include "database.h"

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject *parent = 0);
    void Run();

signals:

public slots:

private:
    Serial *mSerial;
    Database *mDatabase;
};

#endif // MANAGER_H
