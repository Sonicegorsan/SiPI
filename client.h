// client.h
#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>

class Client : public QObject {
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr);

private:
    QTcpSocket *socket;
    QString token;

    void sendRequest(const QJsonObject &requestObj);
    void authenticate(const QString &username, const QString &password);
    void getData(const QString &dataType);
    void sendData(const QJsonObject &data);

private slots:
    void onConnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);
};

#endif // CLIENT_H