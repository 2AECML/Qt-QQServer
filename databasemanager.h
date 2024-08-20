#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();

    bool insertRegisterInfo(const QString& nickname, const QString& password, const QString& phone, QString& hintMessage);
    bool verifyLoginInfo(const QString& account, const  QString& password, QString& hintMessage);

private:
    bool connectToDatabase();
    void closeDatabase();

private:
    QString mHost;
    int mPort;
    QString mUserName;
    QString mPassword;
};

#endif // DATABASEMANAGER_H
