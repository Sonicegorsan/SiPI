// client.cpp
#include "client.h"
#include <QDebug>

const quint16 SERVER_PORT = 12345;

Client::Client(QObject *parent)
    : QObject(parent)
{
    // Настройка соединения с сервером
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(socket, &QTcpSocket::errorOccurred, this, &Client::onErrorOccurred);
    socket->connectToHost("localhost", SERVER_PORT);
}

void Client::authenticate(const QString &username, const QString &password)
{
    QJsonObject request;
    request["action"] = "authenticate";
    request["username"] = username;
    request["password"] = password;

    sendRequest(request);
}

void Client::getData(const QString &dataType)
{
    QJsonObject request;
    request["action"] = "getData";
    request["token"] = token;
    request["dataType"] = dataType;

    sendRequest(request);
}

void Client::sendData(const QJsonObject &data)
{
    QJsonObject request;
    request["action"] = "sendData";
    request["token"] = token;
    request["data"] = data;

    sendRequest(request);
}

void Client::sendRequest(const QJsonObject &requestObj)
{
    QJsonDocument requestDoc(requestObj);
    socket->write(requestDoc.toJson(QJsonDocument::Compact));
    socket->flush();
}

void Client::onConnected()
{
    qInfo() << "Подключено к серверу. Выполняется аутентификация...";

    // Выполнение аутентификации
    authenticate("user1", "password1"); // Замените на реальные данные пользователя
}

void Client::onReadyRead()
{
    QByteArray data = socket->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(data);
    QJsonObject responseObj = responseDoc.object();

    QString status = responseObj.value("status").toString();

    if (status == "success") {
        if (responseObj.contains("token")) {
            // Сохранение токена после аутентификации
            token = responseObj.value("token").toString();
            qInfo() << "Аутентификация успешна. Токен:" << token;

            // После успешной аутентификации можно запросить данные или отправить данные
            // Например, запросить список комнат
            getData("rooms");
        } else if (responseObj.contains("data")) {
            // Обработка полученных данных
            QJsonArray dataArray = responseObj.value("data").toArray();
            qInfo() << "Получены данные:" << dataArray;

            // Например, отправить сообщение в комнату
            if (dataArray.size() > 0 && dataArray.at(0).isObject()) {
                QJsonObject firstRoom = dataArray.at(0).toObject();
                int roomId = firstRoom.value("id").toInt();

                QJsonObject messageData;
                messageData["type"] = "message";
                messageData["room_id"] = roomId;
                messageData["content"] = "Привет из клиента!";

                sendData(messageData);
            }
        } else {
            qInfo() << "Операция выполнена успешно.";
        }
    } else {
        QString message = responseObj.value("message").toString();
        qWarning() << "Ошибка:" << message;
    }
}

void Client::onErrorOccurred(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError)
    qCritical() << "Ошибка соединения:" << socket->errorString();
}