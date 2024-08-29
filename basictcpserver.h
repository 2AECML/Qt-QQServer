#ifndef BASICTCPSERVER_H
#define BASICTCPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QJsonObject>
#include <QByteArray>
#include <QAbstractSocket>
#include <QMap>

class BasicTcpServer : public QTcpServer {
    Q_OBJECT
public:
    explicit BasicTcpServer(quint16 port, QObject *parent = nullptr);
    void startServer();

protected:
    void incomingConnection(qintptr socketDescriptor) override;
    virtual void processData(QTcpSocket* socket, const QByteArray& data) = 0;
    QByteArray appendDelimiter(const QJsonObject& jsonObj);
    QList<QJsonObject> extractDataToJsonList(const QByteArray& data);
    void respondToClient(QTcpSocket* socket, const QJsonObject& jsonObj);
    void respondToClient(QTcpSocket* socket, const QByteArray& data);

protected slots:
    virtual void onReadyRead();
    virtual void onSocketStateChanged(QAbstractSocket::SocketState socketState);

protected:
    QMap<QTcpSocket*, qintptr> mSocketMap;
    quint16 mPort;
};

#endif // BASICTCPSERVER_H
