// server.h
#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QtSql>

class ClientHandler;

class Server : public QTcpServer {
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QSqlDatabase db;
};

#endif // SERVER_H