#include "homeserver.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

HomeServer::HomeServer(QObject *parent)
    : BasicTcpServer(8053, parent) {

}

void HomeServer::processData(QTcpSocket* socket, const QByteArray& data) {

    QList<QJsonObject> jsonObjList = extractDataToJsonList(data);

    for (auto& jsonObj : jsonObjList) {
        // 根据消息类型进行处理
        if (jsonObj["type"] == "user_list") {
            // 如果消息类型是 "user_list"，从数据库获取用户列表并发送回客户端
            QList<BasicUserInfo> list = DatabaseManager::getUserList();
            sendUserList(socket, list);
        } else if (jsonObj["type"] == "user_info") {
            // 如果消息类型是 "user_info"，从数据库获取用户信息并发送回客户端
            QString accountID = jsonObj["id"].toString();
            BasicUserInfo info = DatabaseManager::getUserInfo(accountID);
            sendUserInfo(socket, info);
        } else {
            // 如果消息类型未知，记录调试信息
            qDebug() << "HomeServer: An unknown type of json was received: " << data;
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

    respondToClient(socket, jsonObj);
}

void HomeServer::sendUserInfo(QTcpSocket* socket, const BasicUserInfo& info) {
    QJsonObject jsonObj;
    jsonObj["type"] = "user_info";
    jsonObj["nickname"] = info.nickname;
    jsonObj["id"] = info.id;

    respondToClient(socket, jsonObj);
}
