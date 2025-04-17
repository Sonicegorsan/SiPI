#include "server.h"
#include "clienthandler.h"
#include <QSqlError>
#include <QDebug>

Server::Server(QObject *parent) : QTcpServer(parent) {
    // Инициализируем базу данных
    if (!initDatabase()) {
        qCritical() << "Не удалось инициализировать базу данных";
        exit(EXIT_FAILURE);
    }
}

Server::~Server() {
    db.close();
}

bool Server::startServer(quint16 port) {
    if (!this->listen(QHostAddress::Any, port)) {
        qCritical() << "Не удалось запустить сервер:" << this->errorString();
        return false;
    }
    qDebug() << "Сервер запущен на порту" << port;
    return true;
}

void Server::incomingConnection(qintptr socketDescriptor) {
    // Создаём обработчик для нового подключения
    ClientHandler *client = new ClientHandler(socketDescriptor, db, this);
    connect(client, &ClientHandler::disconnected, client, &ClientHandler::deleteLater);
}

bool Server::initDatabase() {
    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setDatabaseName("postgres");
    db.setUserName("specuser");
    db.setPassword("lemz2020");

    if (!db.open()) {
        qCritical() << "Ошибка подключения к базе данных:" << db.lastError().text();
        return false;
    }
    qDebug() << "Соединение с базой данных установлено";
    return true;
}
