#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QSqlDatabase>

class ClientHandler : public QObject {
    Q_OBJECT
public:
    explicit ClientHandler(qintptr socketDescriptor, QSqlDatabase db, QObject *parent = nullptr);

signals:
    void disconnected();

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket *socket;
    QSqlDatabase db;
    QString username;

    void processData(const QByteArray &data);
    void sendResponse(const QByteArray &data);
};

#endif // CLIENTHANDLER_H
