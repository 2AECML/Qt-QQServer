#include "homeserver.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

HomeServer::HomeServer(QObject *parent)
    : QTcpServer(parent)
    , mPort(8053) {

}

void HomeServer::startServer() {
    if (this->listen(QHostAddress::Any, mPort)) {
        qDebug() << "Home server started!";
    }
    else {
        qDebug() << "Home server could not start!";
        qDebug() << "Error" << this->errorString();
    }
}

void HomeServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QTcpSocket::readyRead, this, &HomeServer::onReadyRead);
    connect(socket, &QTcpSocket::stateChanged, this, &HomeServer::onSocketStateChanged);

    qDebug() << "Client connected: " << socketDescriptor;

    // 记录套接字描述符和套接字对象的映射
    mSocketMap.insert(socket, socket->socketDescriptor());
}

void HomeServer::onReadyRead() {

    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());

    if (socket) {
        QByteArray data = socket->readAll();
        if (data == "CLOSE") {
            socket->close();
        }
        qDebug() << "Data received:" << data;
        processData(socket, data);
    }
}

void HomeServer::onSocketStateChanged(QAbstractSocket::SocketState socketState) {
    if (socketState == QAbstractSocket::UnconnectedState) {
        QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
        if (socket) {
            qintptr socketDescriptor = mSocketMap[socket];
            qDebug() << "Client disconnected: " << socketDescriptor;
            mSocketMap.remove(socket);
            socket->deleteLater();
        }
    }
}

void HomeServer::processData(QTcpSocket* socket, const QByteArray& data) {
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    QJsonObject json = jsonDoc.object();
    if (json["type"] == "user_list") {
        QList<BasicUserInfo> list = DatabaseManager::getUserList();
        sendUserList(socket, list);
    }
    else {
        qDebug() << "HomeServer: An unknown type of json was received";
    }
}

void HomeServer::sendUserList(QTcpSocket* socket, const QList<BasicUserInfo>& list) {
    QJsonArray jsonArray;

    for (const BasicUserInfo& userInfo : list) {
        QJsonObject jsonObject;
        jsonObject["id"] = userInfo.id;
        jsonObject["nickname"] = userInfo.nickname;
        jsonArray.append(jsonObject);
    }

    QJsonObject jsonObj;
    jsonObj["type"] = "user_list";
    jsonObj["data"] = jsonArray;

    QJsonDocument jsonDoc(jsonObj);

    socket->write(jsonDoc.toJson());
}
