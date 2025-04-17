#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QSqlDatabase>

class Server : public QTcpServer {
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    ~Server();

    bool startServer(quint16 port);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QSqlDatabase db;
    bool initDatabase();
};

#endif // SERVER_H
