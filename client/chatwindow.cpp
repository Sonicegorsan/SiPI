#include "chatwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ChatWindow::ChatWindow(int roomId, const QString &roomName, QTcpSocket *socket, QWidget *parent)
    : QWidget(parent), roomId(roomId), roomName(roomName), socket(socket) {
    setupUI();
    requestMessages();

    connect(socket, &QTcpSocket::readyRead, this, &ChatWindow::onReadyRead);
}

void ChatWindow::setupUI() {
    setWindowTitle(roomName);

    messageView = new QTextEdit();
    messageView->setReadOnly(true);

    messageInput = new QLineEdit();
    sendButton = new QPushButton("Отправить");

    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(messageInput);
    inputLayout->addWidget(sendButton);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(messageView);
    mainLayout->addLayout(inputLayout);

    setLayout(mainLayout);

    connect(sendButton, &QPushButton::clicked, this, &ChatWindow::onSendButtonClicked);

    // Инициализируем таймер
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &ChatWindow::requestMessages);
    updateTimer->start(5000); // Обновление каждые 5 секунд
}

void ChatWindow::requestMessages() {
    QJsonObject request;
    request["command"] = "GET_MESSAGES";
    request["room_id"] = roomId;

    QJsonDocument doc(request);
    QByteArray data = doc.toJson();
    socket->write(data);
    socket->flush();
}

void ChatWindow::onReadyRead() {
    QByteArray responseData = socket->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    QJsonObject response = responseDoc.object();

    QString status = response["status"].toString();

    if (status == "OK") {
        if (response.contains("messages")) {
            QJsonArray messagesArray = response["messages"].toArray();
            messageView->clear();
            for (const QJsonValue &value : messagesArray) {
                QJsonObject messageObj = value.toObject();
                QString sender = messageObj["sender"].toString();
                QString content = messageObj["content"].toString();
                QString timestamp = messageObj["timestamp"].toString();
                messageView->append(QString("[%1] %2: %3").arg(timestamp, sender, content));
            }
        }
    } else {
        QString message = response["message"].toString();
        qWarning() << "Ошибка от сервера:" << message;
    }
}

void ChatWindow::onSendButtonClicked() {
    QString content = messageInput->text();
    if (content.isEmpty()) {
        return;
    }

    QJsonObject request;
    request["command"] = "SEND_MESSAGE";
    request["room_id"] = roomId;
    request["content"] = content;

    QJsonDocument doc(request);
    QByteArray data = doc.toJson();
    socket->write(data);
    socket->flush();

    messageInput->clear();
}
