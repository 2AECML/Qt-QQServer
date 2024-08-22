#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>
#include <QList>

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

    id insertRegisterInfo(const QString& nickname, const QString& password, const QString& phone, QString& hintMessage);
    bool verifyLoginInfo(const QString& account, const  QString& password, QString& hintMessage);
    QList<BasicUserInfo> getUserList();

private:
    bool connectToDatabase();
    void closeDatabase();
    bool checkDatabase(QSqlDatabase* db);

private slots:
    void keepConnectionAlive();

private:
    QString mHost;
    int mPort;
    QString mUserName;
    QString mPassword;

    QTimer* mKeepAliveTimer;
};

#endif // DATABASEMANAGER_H
