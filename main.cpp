#include "authserver.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    AuthServer as;
    as.startServer();

    return a.exec();
}
