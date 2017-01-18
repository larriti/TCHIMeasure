#include <QCoreApplication>
#include "manager.h"
#include <QSettings>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("THCI");
    QCoreApplication::setApplicationName("THCI");

    QCoreApplication a(argc, argv);
    Manager mManager;
    mManager.Run();

    return a.exec();
}
