#ifndef HOMESERVER_H
#define HOMESERVER_H

#include "databasemanager.h"
#include "basictcpserver.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>

class HomeServer : public BasicTcpServer {
    Q_OBJECT
public:
    explicit HomeServer(QObject *parent = nullptr);

private:
    void processData(QTcpSocket* socket, const QByteArray& data) override;
    void sendUserList(QTcpSocket* socket, const QList<BasicUserInfo>& list);
    void sendUserInfo(QTcpSocket* socket, const BasicUserInfo& info);
};

#endif // HOMESERVER_H
