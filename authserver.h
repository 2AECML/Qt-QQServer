#ifndef AUTHSERVER_H
#define AUTHSERVER_H

#include "databasemanager.h"
#include "basictcpserver.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QByteArray>
#include <QJsonObject>
#include <QMap>

class AuthServer : public BasicTcpServer {
    Q_OBJECT
public:
    explicit AuthServer(QObject* parent = nullptr);

private:
    void processData(QTcpSocket* socket, const QByteArray& data) override;
    void verifyLogin(QTcpSocket* socket, const QString& account, const QString& password);
    void verifyRegister(QTcpSocket* socket, const QString& nickname, const QString& password, const QString& phone, const QString& vcode);

};

#endif // AUTHSERVER_H
