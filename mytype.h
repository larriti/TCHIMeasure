#ifndef MYTYPE_H
#define MYTYPE_H

#include <QDateTime>
#include <QDir>

#define DATETIME QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
#define LOGPATH QDir::homePath()+"/THCI.log"

#endif // MYTYPE_H
