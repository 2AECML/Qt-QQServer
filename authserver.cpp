#include "authserver.h"
#include <QJsonDocument>

AuthServer::AuthServer(QObject *parent)
    : QTcpServer(parent)
    , mPort(8052){

}

void AuthServer::startServer() {
    if (this->listen(QHostAddress::Any, mPort)) {
        qDebug() << "Auth server started!";
    }
    else {
        qDebug() << "Auth server could not start!";
        qDebug() << "Auth server: Error" << this->errorString();
    }
}

void AuthServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QTcpSocket::readyRead, this, &AuthServer::onReadyRead);
    connect(socket, &QTcpSocket::stateChanged, this, &AuthServer::onSocketStateChanged);

    qDebug() << "Auth server: Client connected: " << socketDescriptor;

    // 记录套接字描述符和套接字对象的映射
    mSocketMap.insert(socket, socket->socketDescriptor());
}

void AuthServer::onReadyRead() {

    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        QByteArray data = socket->readAll();
        if (data == "CLOSE") {
            socket->close();
        }
        // qDebug() << "Data received:" << data;
        processData(socket, data);
    }
}

void AuthServer::onSocketStateChanged(QAbstractSocket::SocketState socketState) {
    if (socketState == QAbstractSocket::UnconnectedState) {
        QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
        if (socket) {
            qintptr socketDescriptor = mSocketMap[socket];
            qDebug() << "Auth server: Client disconnected: " << socketDescriptor;
            mSocketMap.remove(socket);
            socket->deleteLater();
        }
    }
}

void AuthServer::processData(QTcpSocket* socket, const QByteArray &data) {
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    QJsonObject json = jsonDoc.object();
    if (json["type"] == "login") {
        verifyLogin(socket, json["account"].toString(), json["password"].toString());
    }
    else if (json["type"] == "register") {
        verifyRegister(socket, json["nickname"].toString(), json["password"].toString(), json["phone"].toString(), json["vcode"].toString());
    }
    else {
        qDebug() << "AuthServer: An unknown type of json was received";
    }
}

void AuthServer::verifyLogin(QTcpSocket* socket, const QString& account, const QString& password) {
    QString message;
    bool success = DatabaseManager::verifyLoginInfo(account, password, message);
    // qDebug() << message;
    respondToClient(socket, "login", success, message);
}

void AuthServer::verifyRegister(QTcpSocket* socket, const QString& nickname, const QString& password, const QString& phone, const QString& vcode) {
    QString message;
    id accountID = DatabaseManager::insertRegisterInfo(nickname, password, phone, message);
    bool success = accountID != 0 ? true : false;
    // qDebug() << message;
    respondToClient(socket, "register", success, message, accountID);
}

void AuthServer::respondToClient(QTcpSocket* socket, const QString& type, const bool success, const QString& message, const id accountID) {
    QJsonObject json;
    json["type"] = type;
    json["success"] = success;
    json["message"] = message;

    if (type == "register") {
        json["id"] = QString::number(accountID);
    }

    QJsonDocument doc(json);

    socket->write(doc.toJson());
}

