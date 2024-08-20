#ifndef AUTHSERVER_H
#define AUTHSERVER_H

#include "databasemanager.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QByteArray>
#include <QJsonObject>

class AuthServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit AuthServer(QObject* parent = nullptr);
    void startServer();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();
    void onSocketStateChanged(QAbstractSocket::SocketState socketState);

private:
    void processData(QTcpSocket* socket, const QByteArray& data);
    void verifyLogin(QTcpSocket* socket, const QString& account, const QString& password);
    void verifyRegister(QTcpSocket* socket, const QString& nickname, const QString& password, const QString& phone, const QString& vcode);
    void respondToClient(QTcpSocket* socket, const QString& type, const bool success, const QString& message);

private:
    DatabaseManager* mDbManager;
};

#endif // AUTHSERVER_H
