#ifndef BASICWEBSERVER_H
#define BASICWEBSERVER_H

#include <QObject>

class BasicWebServer : public QObject
{
    Q_OBJECT
public:
    explicit BasicWebServer(QObject *parent = nullptr);

signals:
};

#endif // BASICWEBSERVER_H
