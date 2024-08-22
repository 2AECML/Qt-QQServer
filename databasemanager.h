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
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    DatabaseManager *getInstance();
    void releaseInstance();

    id insertRegisterInfo(const QString& nickname, const QString& password, const QString& phone, QString& hintMessage);
    bool verifyLoginInfo(const QString& account, const  QString& password, QString& hintMessage);
    QList<BasicUserInfo> getUserList();

private:
    bool connectToDatabase();
    void closeDatabase();
    bool checkDatabase();

private slots:
    void keepConnectionAlive();

private:
    QString mHost;
    int mPort;
    QString mUserName;
    QString mPassword;

    QTimer* mKeepAliveTimer;

    static DatabaseManager* instance;
    static QMutex mutex;  // 用于管理 DatabaseManager 实例的互斥量
    QMutex dbMutex;       // 用于数据库操作的互斥量
    QSqlDatabase db;
};

#endif // DATABASEMANAGER_H
