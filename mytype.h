#ifndef MYTYPE_H
#define MYTYPE_H

#include <QDateTime>
#include <QDir>

#define DATETIME QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
#define LOGPATH QDir::homePath()+"/App/log/THCI.log"
#define CONFIGPATH QDir::homePath()+"/App/THCI/config.ini"

#endif // MYTYPE_H
