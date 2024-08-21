#include "databasemanager.h"
#include <QCryptographicHash>
#include <QThreadStorage>
#include <QThread>

QThreadStorage<QSqlDatabase*> threadDbStorage;

DatabaseManager::DatabaseManager(QObject* parent)
    : mHost("127.0.0.1")
    , mPort(3306)
    , mUserName("root")
    , mPassword("805284158"){
    connectToDatabase();
}

DatabaseManager::~DatabaseManager() {
    closeDatabase();
}

bool DatabaseManager::insertRegisterInfo(const QString& nickname, const QString& password, const QString& phone, QString& hintMessage) {
    QSqlDatabase* db = threadDbStorage.localData();

    if (!db || !db->isOpen()) {
        qDebug() << "Database: connection closed";
        return false;
    }
    if (!db || !db->isValid()) {
        qDebug() << "Database: connection is not valid";
        return false;
    }

    // 哈希密码
    QByteArray hashedPassword = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);

    QSqlQuery query(*db);
    query.prepare(R"(INSERT INTO db_qq.user_info (nickname, pwd, phone)
                     VALUES(:nickname, :pwd, :phone))");
    query.bindValue(":nickname", nickname);
    query.bindValue(":pwd", hashedPassword.toHex());
    query.bindValue(":phone", phone);

    if (query.exec()) {
        hintMessage = "注册成功";
        return true;
    }
    else {
        QSqlError error = query.lastError();
        if (error.type() == QSqlError::NoError) {
            hintMessage = "注册失败，发生未知错误";
        }
        else if (error.nativeErrorCode() == "1062") {
            hintMessage = "注册失败，该手机号已被注册";
        }
        else {
            hintMessage = error.text();
        }
        return false;
    }
}

bool DatabaseManager::verifyLoginInfo(const QString& account, const QString& password, QString& hintMessage) {
    QSqlDatabase* db = threadDbStorage.localData();

    if (!db || !db->isOpen()) {
        qDebug() << "Database: connection closed";
        return false;
    }
    if (!db || !db->isValid()) {
        qDebug() << "Database: connection is not valid";
        return false;
    }

    QSqlQuery query(*db);

    query.prepare(R"(SELECT * FROM db_qq.user_info
                     WHERE phone = :account OR id = :account)");
    query.bindValue(":account", account);

    if (query.exec() && query.next()) {
        // 获取数据库中存储的哈希密码
        QByteArray storedHashedPassword = QByteArray::fromHex(query.value("pwd").toByteArray());

        // 对用户输入的密码进行哈希处理
        QByteArray inputHashedPassword = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);

        if (storedHashedPassword == inputHashedPassword) {
            hintMessage = "登陆成功";
            return true;
        }
        else {
            hintMessage = "登陆失败，账号或密码错误";
            return false;
        }
    }
    else {
        hintMessage = "登陆失败，不存在该账号";
        return false;
    }
}

bool DatabaseManager::connectToDatabase() {
    if (threadDbStorage.hasLocalData()) {
        qDebug() << "Current thread has created a database connection, failed";
        return false;
    }

    QString connectionName = QString("connection_%1").arg((quintptr)QThread::currentThreadId());
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", connectionName);
    db.setHostName(mHost);
    db.setPort(mPort);
    db.setUserName(mUserName);
    db.setPassword(mPassword);


    if (!db.open()) {
        qDebug() << "Error: connection with database failed";
        qDebug() << db.lastError().text();
        return false;
    }
    else {
        qDebug() << "Database: connection ok";
        threadDbStorage.setLocalData(new QSqlDatabase(db));
        return true;
    }
}

void DatabaseManager::closeDatabase() {
    if (threadDbStorage.hasLocalData()) {
        QSqlDatabase* db = threadDbStorage.localData();
        if (db->isOpen()) {
            db->close();
        }
        QString connectionName = db->connectionName();
        delete db;
        threadDbStorage.setLocalData(nullptr);
        QSqlDatabase::removeDatabase(connectionName);
    }
}

