#include "authserver.h"
#include "homeserver.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    AuthServer as;
    as.startServer();

    HomeServer hs;
    hs.startServer();

    return a.exec();
}
