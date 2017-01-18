#include <QCoreApplication>
#include "manager.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Manager mManager;
    mManager.Run();

    return a.exec();
}
