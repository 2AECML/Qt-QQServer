#include "databasemanager.h"
#include "authserver.h"
#include "homeserver.h"
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    DatabaseManager::initialize();

    AuthServer as;
    as.startServer();

    HomeServer hs;
    hs.startServer();

    int ret = a.exec();

    DatabaseManager::finalize();

    return ret;
}
