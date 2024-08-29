#include "authserver.h"
#include <QJsonDocument>

AuthServer::AuthServer(QObject *parent)
    : BasicTcpServer(8052, parent){

}

void AuthServer::processData(QTcpSocket* socket, const QByteArray& data) {

    QList<QJsonObject> jsonObjList = extractDataToJsonList(data);

    for (auto& jsonObj : jsonObjList) {
        // 根据消息类型进行处理
        if (jsonObj["type"] == "login") {
            verifyLogin(socket, jsonObj["account"].toString(), jsonObj["password"].toString());
        }
        else if (jsonObj["type"] == "register") {
            verifyRegister(socket, jsonObj["nickname"].toString(), jsonObj["password"].toString(), jsonObj["phone"].toString(), jsonObj["vcode"].toString());
        }
        else {
            qDebug() << "AuthServer: An unknown type of json was received: " << data;
        }
    }
}

void AuthServer::verifyLogin(QTcpSocket* socket, const QString& account, const QString& password) {
    QString message;
    id accountID = DatabaseManager::verifyLoginInfo(account, password, message);
    bool success = accountID;
    // qDebug() << message;

    QJsonObject jsonObj;
    jsonObj["type"] = "login";
    jsonObj["success"] = success;
    jsonObj["message"] = message;
    jsonObj["id"] = QString::number(accountID);

    respondToClient(socket, jsonObj);
}

void AuthServer::verifyRegister(QTcpSocket* socket, const QString& nickname, const QString& password, const QString& phone, const QString& vcode) {
    QString message;
    id accountID = DatabaseManager::insertRegisterInfo(nickname, password, phone, message);
    bool success = accountID;
    // qDebug() << message;

    QJsonObject jsonObj;
    jsonObj["type"] = "register";
    jsonObj["success"] = success;
    jsonObj["message"] = message;
    jsonObj["id"] = QString::number(accountID);

    respondToClient(socket, jsonObj);
}

