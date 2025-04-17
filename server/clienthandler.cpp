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

void ClientHandler::onReadyRead() {
    QByteArray data = socket->readAll();
    processData(data);
}

void ClientHandler::onDisconnected() {
    qDebug() << "Клиент отключился:" << socket->peerAddress().toString();
    emit disconnected();
}

void ClientHandler::processData(const QByteArray &data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Получены некорректные данные";
        return;
    }

    QJsonObject request = doc.object();
    QString command = request["command"].toString();

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
    else if (command == "GET_MESSAGES") {
        int roomId = request["room_id"].toInt();

        QSqlQuery query(db);
        query.prepare("SELECT messages.content, messages.timestamp, users.username AS sender "
                      "FROM messages "
                      "JOIN users ON messages.sender_id = users.id "
                      "WHERE messages.room_id = :room_id "
                      "ORDER BY messages.timestamp ASC");
        query.bindValue(":room_id", roomId);

        if (query.exec()) {
            QJsonArray messagesArray;
            while (query.next()) {
                QJsonObject messageObj;
                messageObj["content"] = query.value("content").toString();
                messageObj["timestamp"] = query.value("timestamp").toDateTime().toString(Qt::ISODate);
                messageObj["sender"] = query.value("sender").toString();
                messagesArray.append(messageObj);
            }

            QJsonObject response;
            response["status"] = "OK";
            response["messages"] = messagesArray;
            sendResponse(QJsonDocument(response).toJson());
        } else {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Ошибка получения сообщений";
            sendResponse(QJsonDocument(response).toJson());
            qWarning() << "Ошибка получения сообщений:" << query.lastError().text();
        }
    }
    else if (command == "GET_TASKS") {
        QString startDateStr = request["start_date"].toString();
        QString endDateStr = request["end_date"].toString();

        QDateTime startDate = QDateTime::fromString(startDateStr, Qt::ISODate);
        QDateTime endDate = QDateTime::fromString(endDateStr, Qt::ISODate);

        QSqlQuery query(db);
        query.prepare("SELECT id, description, deadline FROM tasks "
                      "WHERE deadline BETWEEN :start_date AND :end_date");
        query.bindValue(":start_date", startDate);
        query.bindValue(":end_date", endDate);

        if (query.exec()) {
            QJsonArray tasksArray;
            while (query.next()) {
                QJsonObject taskObj;
                taskObj["id"] = query.value("id").toInt();
                taskObj["description"] = query.value("description").toString();
                taskObj["deadline"] = query.value("deadline").toDateTime().toString(Qt::ISODate);
                tasksArray.append(taskObj);
            }

            QJsonObject response;
            response["status"] = "OK";
            response["tasks"] = tasksArray;
            sendResponse(QJsonDocument(response).toJson());
        } else {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Ошибка получения задач";
            sendResponse(QJsonDocument(response).toJson());
            qWarning() << "Ошибка получения задач:" << query.lastError().text();
        }
    }
    else if (command == "ADD_TASK") {
        int roomId = request["room_id"].toInt();
        QString assignedToUsername = request["assigned_to"].toString();
        QString description = request["description"].toString();
        QString deadlineStr = request["deadline"].toString();

        if (this->username.isEmpty()) {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Пользователь не аутентифицирован";
            sendResponse(QJsonDocument(response).toJson());
            return;
        }

        // Проверяем роль пользователя
        QSqlQuery roleQuery(db);
        roleQuery.prepare("SELECT role FROM users WHERE username = :username");
        roleQuery.bindValue(":username", this->username);

        if (roleQuery.exec() && roleQuery.next()) {
            QString role = roleQuery.value("role").toString();
            if (role != "manager" && role != "administrator") {
                QJsonObject response;
                response["status"] = "ERROR";
                response["message"] = "Недостаточно прав для добавления задачи";
                sendResponse(QJsonDocument(response).toJson());
                return;
            }
        } else {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Ошибка доступа";
            sendResponse(QJsonDocument(response).toJson());
            qWarning() << "Ошибка проверки роли пользователя:" << roleQuery.lastError().text();
            return;
        }

        // Получаем ID назначенного пользователя
        QSqlQuery userQuery(db);
        userQuery.prepare("SELECT id FROM users WHERE username = :username");
        userQuery.bindValue(":username", assignedToUsername);

        int assignedToId = -1;
        if (userQuery.exec() && userQuery.next()) {
            assignedToId = userQuery.value("id").toInt();
        } else {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Назначенный пользователь не найден";
            sendResponse(QJsonDocument(response).toJson());
            qWarning() << "Назначенный пользователь не найден:" << assignedToUsername;
            return;
        }

        // Вставляем задачу
        QSqlQuery insertQuery(db);
        insertQuery.prepare("INSERT INTO tasks (room_id, assigned_to, description, status, deadline) VALUES (:room_id, :assigned_to, :description, :status, :deadline)");
        insertQuery.bindValue(":room_id", roomId);
        insertQuery.bindValue(":assigned_to", assignedToId);
        insertQuery.bindValue(":description", description);
        insertQuery.bindValue(":status", "Новая");
        insertQuery.bindValue(":deadline", QDateTime::fromString(deadlineStr, Qt::ISODate));

        if (insertQuery.exec()) {
            QJsonObject response;
            response["status"] = "OK";
            response["message"] = "Задача добавлена";
            sendResponse(QJsonDocument(response).toJson());
            qDebug() << "Задача добавлена менеджером" << this->username;
        } else {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Ошибка добавления задачи";
            sendResponse(QJsonDocument(response).toJson());
            qWarning() << "Ошибка добавления задачи:" << insertQuery.lastError().text();
        }
    }
    else if (command == "GET_REPORTS") {
        // Проверяем роль пользователя
        QSqlQuery roleQuery(db);
        roleQuery.prepare("SELECT role FROM users WHERE username = :username");
        roleQuery.bindValue(":username", this->username);

        if (roleQuery.exec() && roleQuery.next()) {
            QString role = roleQuery.value("role").toString();
            if (role != "owner") {
                QJsonObject response;
                response["status"] = "ERROR";
                response["message"] = "Недостаточно прав для получения отчётов";
                sendResponse(QJsonDocument(response).toJson());
                return;
            }
        } else {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Ошибка доступа";
            sendResponse(QJsonDocument(response).toJson());
            qWarning() << "Ошибка проверки роли пользователя:" << roleQuery.lastError().text();
            return;
        }

        // Генерация отчёта
        QSqlQuery reportQuery(db);
        reportQuery.prepare("SELECT COUNT(*) AS completed_tasks FROM tasks WHERE status = 'Завершена'");

        if (reportQuery.exec() && reportQuery.next()) {
            int completedTasks = reportQuery.value("completed_tasks").toInt();

            QJsonObject report;
            report["completed_tasks"] = completedTasks;

            QJsonObject response;
            response["status"] = "OK";
            response["report"] = report;
            sendResponse(QJsonDocument(response).toJson());
        } else {
            QJsonObject response;
            response["status"] = "ERROR";
            response["message"] = "Ошибка генерации отчёта";
            sendResponse(QJsonDocument(response).toJson());
            qWarning() << "Ошибка генерации отчёта:" << reportQuery.lastError().text();
        }
    }
    else {
        QJsonObject response;
        response["status"] = "ERROR";
        response["message"] = "Неизвестная команда";
        sendResponse(QJsonDocument(response).toJson());
        qWarning() << "Получена неизвестная команда";
    }
}

void ClientHandler::sendResponse(const QByteArray &data) {
    socket->write(data);
    socket->flush();
}
