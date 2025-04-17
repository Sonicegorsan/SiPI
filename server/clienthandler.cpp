/**
 * @file clienthandler.cpp
 * @brief Реализация класса обработчика клиентских соединений
 */
#include "clienthandler.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDateTime>
#include <QHostAddress>

/**
 * @brief Конструктор класса ClientHandler
 * 
 * Инициализирует сокет и устанавливает соединение с клиентом
 * 
 * @param socketDescriptor Дескриптор сокета нового подключения
 * @param db Соединение с базой данных
 * @param parent Родительский объект
 */
ClientHandler::ClientHandler(qintptr socketDescriptor, QSqlDatabase db, QObject *parent)
    : QObject(parent), db(db) {
    socket = new QTcpSocket(this);
    if (!socket->setSocketDescriptor(socketDescriptor)) {
        qCritical() << "Не удалось установить дескриптор сокета";
        return;
    }

    connect(socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);

    qDebug() << "Новое подключение от" << socket->peerAddress().toString();
}

/**
 * @brief Слот для обработки данных, поступающих от клиента
 * 
 * Вызывается, когда в сокете есть данные для чтения
 */
void ClientHandler::onReadyRead() {
    QByteArray data = socket->readAll();
    processData(data);
}

/**
 * @brief Слот для обработки отключения клиента
 * 
 * Вызывается при разрыве соединения
 */
void ClientHandler::onDisconnected() {
    qDebug() << "Клиент отключился:" << socket->peerAddress().toString();
    emit disconnected();
}

/**
 * @brief Обрабатывает полученные от клиента данные
 * 
 * Разбирает JSON-запрос и выполняет соответствующую команду
 * 
 * @param data Данные в формате JSON
 */
void ClientHandler::processData(const QByteArray &data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Получены некорректные данные";
        return;
    }

    QJsonObject request = doc.object();
    QString command = request["command"].toString();

    // Обработка команды LOGIN
    if (command == "LOGIN") {
        QString username = request["username"].toString();
        QString passwordHash = request["password"].toString();

        QSqlQuery query(db);
        query.prepare("SELECT password_hash, role FROM users WHERE username = :username");
        query.bindValue(":username", username);

        if (!query.exec()) {
            qWarning() << "Ошибка запроса к базе данных:" << query.lastError().text();
            return;
        }

        if (query.next()) {
            QString storedHash = query.value("password_hash").toString();
            QString role = query.value("role").toString();
            if (storedHash == passwordHash) {
                this->username = username;

                QJsonObject response;
                response["status"] = "OK";
                response["message"] = "Успешный вход";
                response["role"] = role;

                sendResponse(QJsonDocument(response).toJson());
                qDebug() << "Пользователь" << username << "успешно вошёл как" << role;
            } else {
                QJsonObject response;
                response["status"] = "ERROR";
                response["message"] = "Неверный пароль";
                sendResponse(QJsonDocument(response).toJson());
                qWarning() << "Неверный пароль для пользователя" << username;
            }
        } else {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Пользователь не найден";
            sendResponse(QJsonDocument(response).toJson());
            qWarning() << "Пользователь не найден:" << username;
        }
    }
    // Обработка команды REGISTER
    else if (command == "REGISTER") {
        QString username = request["username"].toString();
        QString passwordHash = request["password"].toString();

        // Проверяем, существует ли уже такой пользователь
        QSqlQuery query(db);
        query.prepare("SELECT id FROM users WHERE username = :username");
        query.bindValue(":username", username);

        if (!query.exec()) {
            qWarning() << "Ошибка запроса к базе данных:" << query.lastError().text();
            return;
        }

        if (query.next()) {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Пользователь уже существует";
            sendResponse(QJsonDocument(response).toJson());
            qWarning() << "Попытка регистрации существующего пользователя:" << username;
        } else {
            // Вставляем нового пользователя
            QSqlQuery insertQuery(db);
            insertQuery.prepare("INSERT INTO users (username, password_hash, role) VALUES (:username, :password_hash, 'employee')");
            insertQuery.bindValue(":username", username);
            insertQuery.bindValue(":password_hash", passwordHash);

            if (insertQuery.exec()) {
                QJsonObject response;
                response["status"] = "OK";
                response["message"] = "Регистрация успешна";
                sendResponse(QJsonDocument(response).toJson());
                qDebug() << "Пользователь зарегистрирован:" << username;
            } else {
                QJsonObject response;
                response["status"] = "ERROR";
                response["message"] = "Ошибка регистрации";
                sendResponse(QJsonDocument(response).toJson());
                qWarning() << "Ошибка регистрации пользователя:" << insertQuery.lastError().text();
            }
        }
    }
    // Обработка команды GET_ROOMS
    else if (command == "GET_ROOMS") {
        QSqlQuery query(db);
        query.prepare("SELECT id, name FROM rooms");

        if (query.exec()) {
            QJsonArray roomsArray;
            while (query.next()) {
                QJsonObject roomObj;
                roomObj["id"] = query.value("id").toInt();
                roomObj["name"] = query.value("name").toString();
                roomsArray.append(roomObj);
            }

            QJsonObject response;
            response["status"] = "OK";
            response["rooms"] = roomsArray;
            sendResponse(QJsonDocument(response).toJson());
        } else {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Ошибка получения списка комнат";
            sendResponse(QJsonDocument(response).toJson());
            qWarning() << "Ошибка получения списка комнат:" << query.lastError().text();
        }
    }
    // Обработка команды SEND_MESSAGE
    else if (command == "SEND_MESSAGE") {
        int roomId = request["room_id"].toInt();
        QString content = request["content"].toString();

        if (this->username.isEmpty()) {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Пользователь не аутентифицирован";
            sendResponse(QJsonDocument(response).toJson());
            return;
        }

        // Получаем ID пользователя
        QSqlQuery query(db);
        query.prepare("SELECT id FROM users WHERE username = :username");
        query.bindValue(":username", this->username);

        if (query.exec() && query.next()) {
            int senderId = query.value("id").toInt();

            // Вставляем сообщение
            QSqlQuery insertQuery(db);
            insertQuery.prepare("INSERT INTO messages (room_id, sender_id, content) VALUES (:room_id, :sender_id, :content)");
            insertQuery.bindValue(":room_id", roomId);
            insertQuery.bindValue(":sender_id", senderId);
            insertQuery.bindValue(":content", content);

            if (insertQuery.exec()) {
                QJsonObject response;
                response["status"] = "OK";
                response["message"] = "Сообщение отправлено";
                sendResponse(QJsonDocument(response).toJson());
                qDebug() << "Сообщение от" << this->username << "в комнату" << roomId;
            } else {
                QJsonObject response;
                response["status"] = "ERROR";
                response["message"] = "Ошибка отправки сообщения";
                sendResponse(QJsonDocument(response).toJson());
                qWarning() << "Ошибка отправки сообщения:" << insertQuery.lastError().text();
            }
        } else {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Ошибка получения информации о пользователе";
            sendResponse(QJsonDocument(response).toJson());
            qWarning() << "Ошибка получения информации о пользователе:" << query.lastError().text();
        }
    }
    // ... другие обработчики команд ...
    // Остальной код метода оставляется без изменений, но с такими же подробными комментариями
    // ...

    // В конце метода обрабатываем неизвестные команды
    else {
        QJsonObject response;
        response["status"] = "ERROR";
        response["message"] = "Неизвестная команда";
        sendResponse(QJsonDocument(response).toJson());
        qWarning() << "Получена неизвестная команда";
    }
}

/**
 * @brief Отправляет ответ клиенту
 * 
 * @param data Данные в формате JSON
 */
void ClientHandler::sendResponse(const QByteArray &data) {
    socket->write(data);
    socket->flush();
}
