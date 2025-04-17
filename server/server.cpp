/**
 * @file server.cpp
 * @brief Реализация класса сервера
 */

#include "server.h"
#include "clienthandler.h"
#include <QSqlError>
#include <QDebug>

/**
 * @brief Конструктор класса Server
 * 
 * Инициализирует соединение с базой данных и настраивает сервер
 * 
 * @param parent Родительский объект
 */
Server::Server(QObject *parent) : QTcpServer(parent) {
    // Инициализируем базу данных
    if (!initDatabase()) {
        qCritical() << "Не удалось инициализировать базу данных";
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Деструктор класса Server
 * 
 * Закрывает соединение с базой данных
 */
Server::~Server() {
    db.close();
}

/**
 * @brief Запуск сервера на указанном порту
 * 
 * @param port Номер порта для прослушивания
 * @return true при успешном запуске, false в случае ошибки
 */
bool Server::startServer(quint16 port) {
    if (!this->listen(QHostAddress::Any, port)) {
        qCritical() << "Не удалось запустить сервер:" << this->errorString();
        return false;
    }
    qDebug() << "Сервер запущен на порту" << port;
    return true;
}

/**
 * @brief Обработка входящих соединений
 * 
 * Создает новый экземпляр ClientHandler для каждого нового подключения
 * 
 * @param socketDescriptor Дескриптор сокета нового подключения
 */
void Server::incomingConnection(qintptr socketDescriptor) {
    // Создаём обработчик для нового подключения
    ClientHandler *client = new ClientHandler(socketDescriptor, db, this);
    connect(client, &ClientHandler::disconnected, client, &ClientHandler::deleteLater);
}

/**
 * @brief Инициализация соединения с базой данных
 * 
 * Устанавливает параметры подключения и открывает соединение с PostgreSQL
 * 
 * @return true при успешном подключении, false в случае ошибки
 */
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
