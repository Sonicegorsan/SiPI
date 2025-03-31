// server.cpp
#include "server.h"
#include "clienthandler.h"
#include <QDebug>

// Порт для связи между клиентом и сервером
const quint16 SERVER_PORT = 12345;

Server::Server(QObject *parent)
    : QTcpServer(parent)
{
    // Инициализация базы данных
    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setDatabaseName("mydb");
    db.setUserName("myuser");
    db.setPassword("mypassword");
    if (!db.open()) {
        qCritical() << "Не удалось подключиться к базе данных:" << db.lastError().text();
        return;
    }

    // Запуск сервера
    if (!this->listen(QHostAddress::Any, SERVER_PORT)) {
        qCritical() << "Не удалось запустить сервер:" << this->errorString();
        return;
    }
    qInfo() << "Сервер запущен на порту" << SERVER_PORT;
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    // При новом подключении создаем объект обработчика
    ClientHandler *client = new ClientHandler(socketDescriptor, db, this);
    connect(client, &ClientHandler::finished, client, &ClientHandler::deleteLater);
    client->start();
}