#ifndef MYTYPE_H
#define MYTYPE_H

#include <QDateTime>

#ifndef DATETIME
#define DATETIME QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss")
#endif

#endif // MYTYPE_H
