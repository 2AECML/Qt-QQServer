#include "databasemanager.h"
#include <QCryptographicHash>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

QSqlDatabase DatabaseManager::mDb;
QMutex DatabaseManager::mDbMutex;
QString DatabaseManager::mHost = "127.0.0.1";
quint16 DatabaseManager::mPort = 3306;
QString DatabaseManager::mUserName = "root";
QString DatabaseManager::mPassword = "805284158";
QTimer DatabaseManager::mKeepAliveTimer;

bool DatabaseManager::initialize() {
    QMutexLocker locker(&mDbMutex);

    if (mDb.isOpen()) {
        qDebug() << "Database already connected";
        return true;
    }

    mDb = QSqlDatabase::addDatabase("QMYSQL");
    mDb.setHostName(mHost);
    mDb.setPort(mPort);
    mDb.setUserName(mUserName);
    mDb.setPassword(mPassword);

    if (!mDb.open()) {
        qDebug() << "Error: connection with database failed";
        qDebug() << "Database error:" << mDb.lastError().text();
        return false;
    } else {
        qDebug() << "Database: connection ok, name:" << mDb.connectionName();
        setupKeepAlive();
        return true;
    }
}

void DatabaseManager::finalize() {
    QMutexLocker locker(&mDbMutex);

    if (mDb.isOpen()) {
        mDb.close();
        qDebug() << "Database connection closed";
    }
    QSqlDatabase::removeDatabase(mDb.connectionName());
    qDebug() << "Database connection removed";
}

id DatabaseManager::insertRegisterInfo(const QString& nickname, const QString& password, const QString& phone, QString& hintMessage) {
    QMutexLocker locker(&mDbMutex);

    if (!checkDatabase()) {
        hintMessage = "数据库连接无效";
        return 0;
    }

    // 哈希密码
    QByteArray hashedPassword = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);

    QSqlQuery query(mDb);
    query.prepare(R"(INSERT INTO db_qq.user_info (nickname, pwd, phone)
                     VALUES(:nickname, :pwd, :phone))");
    query.bindValue(":nickname", nickname);
    query.bindValue(":pwd", hashedPassword.toHex());
    query.bindValue(":phone", phone);

    if (query.exec()) {
        // 获取自动生成的 ID
        QSqlQuery idQuery(mDb);
        idQuery.exec("SELECT LAST_INSERT_ID()");
        if (idQuery.next()) {
            id generatedId = idQuery.value(0).toLongLong();
            hintMessage = "注册成功，QQ账号: " + QString::number(generatedId);
            qDebug() << "Insert query executed successfully, generated ID:" << generatedId;
            return generatedId;
        }
    } else {
        QSqlError error = query.lastError();
        qDebug() << "Insert query failed:" << error.text();
        if (error.type() == QSqlError::NoError) {
            hintMessage = "注册失败，发生未知错误";
        } else if (error.nativeErrorCode() == "1062") {
            hintMessage = "注册失败，该手机号已被注册";
        } else {
            hintMessage = error.text();
        }
    }
    return 0;
}

bool DatabaseManager::verifyLoginInfo(const QString& account, const QString& password, QString& hintMessage) {
    QMutexLocker locker(&mDbMutex);

    if (!checkDatabase()) {
        hintMessage = "数据库连接无效";
        return false;
    }

    QSqlQuery query(mDb);
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
            qDebug() << "Login query executed successfully";
            return true;
        } else {
            hintMessage = "登陆失败，账号或密码错误";
            qDebug() << "Login query failed: incorrect password";
            return false;
        }
    } else {
        hintMessage = "登陆失败，不存在该账号";
        qDebug() << "Login query failed: account not found";
        return false;
    }
}

QList<BasicUserInfo> DatabaseManager::getUserList() {
    QMutexLocker locker(&mDbMutex);

    if (!checkDatabase()) {
        return QList<BasicUserInfo>();
    }

    QList<BasicUserInfo> result;

    QSqlQuery query(mDb);
    query.prepare(R"(SELECT * FROM db_qq.user_info)");

    if (query.exec()) {
        while (query.next()) {
            BasicUserInfo info;
            info.id = query.value("id").toString();
            info.nickname = query.value("nickname").toString();
            result.append(info);
        }
    } else {
        qDebug() << "Failed to execute query:" << query.lastError().text();
    }

    return result;
}

bool DatabaseManager::checkDatabase() {

    if (!mDb.isOpen()) {
        qDebug() << "Database: connection closed";
        return false;
    }
    if (!mDb.isValid()) {
        qDebug() << "Database: connection is not valid";
        return false;
    }

    qDebug() << "Database: connection is valid";
    return true;
}

void DatabaseManager::setupKeepAlive() {
    connect(&mKeepAliveTimer, &QTimer::timeout, &DatabaseManager::keepConnectionAlive);
    mKeepAliveTimer.start(10000);  // 每10秒发送一次保活查询
}

void DatabaseManager::keepConnectionAlive() {

    if (mDb.isOpen() && mDb.isValid()) {
        QSqlQuery query(mDb);
        query.exec("SELECT 1");  // 简单的查询以保持连接活跃
    }
}
