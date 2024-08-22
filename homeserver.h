#ifndef HOMESERVER_H
#define HOMESERVER_H

#include "databasemanager.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QString>

class HomeServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit HomeServer(QObject *parent = nullptr);
    void startServer();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);

private:
    void processData(QTcpSocket* socket, const QByteArray& data);
    void sendUserList(QTcpSocket* socket, const QList<BasicUserInfo>& list);

private:
    QMap<QTcpSocket*, qintptr> mSocketMap;
    quint16 mPort;
};

#endif // HOMESERVER_H
