#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QList>
#include <QMutex>
#include <QMutexLocker>

typedef int64_t id;

struct BasicUserInfo {
    QString nickname;
    QString id;
};

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static bool initialize();
    static void finalize();
    static id insertRegisterInfo(const QString& nickname, const QString& password, const QString& phone, QString& hintMessage);
    static id verifyLoginInfo(const QString& account, const  QString& password, QString& hintMessage);
    static QList<BasicUserInfo> getUserList();
    static BasicUserInfo getUserInfo(const QString& accountID);

private:
    DatabaseManager() = delete; // 禁止创建实例
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    static bool checkDatabase();
    static void setupKeepAlive();
    static void keepConnectionAlive();

private:
    static QMutex mDbMutex;       // 用于数据库操作的互斥量
    static QSqlDatabase mDb;

    static QString mHost;
    static quint16 mPort;
    static QString mUserName;
    static QString mPassword;

    static QTimer mKeepAliveTimer;

};

#endif // DATABASEMANAGER_H
