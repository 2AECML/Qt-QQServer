#include "basicserver.h"
#include <QJsonDocument>

BasicServer::BasicServer(quint16 port, QObject* parent)
    : mPort(port)
    , QTcpServer(parent) {

}

void BasicServer::startServer() {
    if (this->listen(QHostAddress::Any, mPort)) {
        qDebug() << "*** server started!";
    }
    else {
        qDebug() << "*** server could not start!";
        qDebug() << "*** server: Error" << this->errorString();
    }
}

void BasicServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket* socket = new QTcpSocket(this);
    socket->setSocketDescriptor(socketDescriptor);

    connect(socket, &QTcpSocket::readyRead, this, &BasicServer::onReadyRead);
    connect(socket, &QTcpSocket::stateChanged, this, &BasicServer::onSocketStateChanged);

    qDebug() << "*** server: Client connected: " << socketDescriptor;

    // 记录套接字描述符和套接字对象的映射
    mSocketMap.insert(socket, socket->socketDescriptor());
}

void BasicServer::onReadyRead() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        QByteArray data = socket->readAll();
        if (data == "CLOSE") {
            socket->close();
            return;
        }
        // qDebug() << "Data received:" << data;
        processData(socket, data);
    }
}

void BasicServer::onSocketStateChanged(QAbstractSocket::SocketState socketState) {
    if (socketState == QAbstractSocket::UnconnectedState) {
        QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
        if (socket) {
            qintptr socketDescriptor = mSocketMap[socket];
            qDebug() << "*** server: Client disconnected: " << socketDescriptor;
            mSocketMap.remove(socket);
            socket->deleteLater();
        }
    }
}

QByteArray BasicServer::appendDelimiter(const QJsonObject &jsonObj) {
    QJsonDocument jsonDoc(jsonObj);
    // 将 QJsonDocument 转换为字节数组，并在其末尾添加分隔符 "\r\n\r\n"
    QByteArray data = jsonDoc.toJson() + "\r\n\r\n";
    return data;
}

QList<QJsonObject> BasicServer::extractDataToJsonList(const QByteArray &data) {
    // 储存Json的列表
    QList<QJsonObject> result;

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
        QJsonObject jsonObj = jsonDoc.object();

        result.append(jsonObj);
    }

    return result;
}

void BasicServer::respondToClient(QTcpSocket* socket, const QJsonObject& jsonObj) {
    QByteArray data = appendDelimiter(jsonObj);

    socket->write(data);
}

void BasicServer::respondToClient(QTcpSocket* socket, const QByteArray& data) {
    socket->write(data);
}
