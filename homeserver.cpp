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
        qDebug() << "HomeServer: Error" << this->errorString();
    }
}

void HomeServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QTcpSocket::readyRead, this, &HomeServer::onReadyRead);
    connect(socket, &QTcpSocket::stateChanged, this, &HomeServer::onSocketStateChanged);

    qDebug() << "HomeServer: Client connected: " << socketDescriptor;

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
        // qDebug() << "HomeServer: Data received:" << data;
        processData(socket, data);
    }
}

void HomeServer::onSocketStateChanged(QAbstractSocket::SocketState socketState) {
    if (socketState == QAbstractSocket::UnconnectedState) {
        QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
        if (socket) {
            qintptr socketDescriptor = mSocketMap[socket];
            qDebug() << "HomeServer: Client disconnected: " << socketDescriptor;
            mSocketMap.remove(socket);
            socket->deleteLater();
        }
    }
}

void HomeServer::processData(QTcpSocket* socket, const QByteArray& data) {
    // 静态缓冲区，用于存储接收到的数据
    static QByteArray buffer;

    // 将新接收到的数据追加到缓冲区
    buffer.append(data);

    // 处理缓冲区中的数据，直到找不到分隔符为止
    while (buffer.contains("\r\n\r\n")) {
        // 找到分隔符的位置
        int index = buffer.indexOf("\r\n\r\n");

        // 提取分隔符之前的部分作为一个完整的消息
        QByteArray message = buffer.left(index).trimmed();

        // 从缓冲区中移除已经处理的消息
        buffer.remove(0, index + 4);

        // 解析 JSON 消息
        QJsonDocument jsonDoc = QJsonDocument::fromJson(message);
        QJsonObject json = jsonDoc.object();

        // 根据消息类型进行处理
        if (json["type"] == "user_list") {
            // 如果消息类型是 "user_list"，从数据库获取用户列表并发送回客户端
            QList<BasicUserInfo> list = DatabaseManager::getUserList();
            sendUserList(socket, list);
        } else if (json["type"] == "user_info") {
            // 如果消息类型是 "user_info"，从数据库获取用户信息并发送回客户端
            QString accountID = json["id"].toString();
            BasicUserInfo info = DatabaseManager::getUserInfo(accountID);
            sendUserInfo(socket, info);
        } else {
            // 如果消息类型未知，记录调试信息
            qDebug() << "HomeServer: An unknown type of json was received: " << message;
        }
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
    jsonObj["list"] = jsonArray;

    QJsonDocument jsonDoc(jsonObj);

    // 将 QJsonDocument 转换为字节数组，并在其末尾添加分隔符 "\r\n\r\n"
    QByteArray data = jsonDoc.toJson() + "\r\n\r\n";

    socket->write(data);
}

void HomeServer::sendUserInfo(QTcpSocket* socket, const BasicUserInfo& info) {
    QJsonObject jsonObj;
    jsonObj["type"] = "user_info";
    jsonObj["nickname"] = info.nickname;
    jsonObj["id"] = info.id;

    QJsonDocument jsonDoc(jsonObj);

    // 将 QJsonDocument 转换为字节数组，并在其末尾添加分隔符 "\r\n\r\n"
    QByteArray data = jsonDoc.toJson() + "\r\n\r\n";

    socket->write(data);
}
