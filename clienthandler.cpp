// clienthandler.cpp
#include "clienthandler.h"
#include <QDateTime>
#include <QUuid>
#include <QCryptographicHash>
#include <QSqlError>

ClientHandler::ClientHandler(qintptr descriptor, QSqlDatabase database, QObject *parent)
    : QThread(parent), socketDescriptor(descriptor), db(database)
{
}

void ClientHandler::run()
{
    QTcpSocket socket;
    if (!socket.setSocketDescriptor(socketDescriptor)) {
        emit error(socket.error());
        return;
    }

    // Обработка запросов от клиента
    while (socket.state() == QTcpSocket::ConnectedState) {
        if (!socket.waitForReadyRead(30000)) {
            // Если данные не поступили в течение 30 секунд, завершаем соединение
            break;
        }

        QByteArray data = socket.readAll();
        QJsonDocument requestDoc = QJsonDocument::fromJson(data);
        QJsonObject requestObj = requestDoc.object();

        QJsonObject responseObj;

        QString action = requestObj.value("action").toString();

        if (action == "authenticate") {
            // Обработка аутентификации
            QString username = requestObj.value("username").toString();
            QString password = requestObj.value("password").toString();

            QSqlQuery query(db);
            query.prepare("SELECT id, password_hash, role FROM users WHERE username = :username");
            query.bindValue(":username", username);
            if (query.exec() && query.next()) {
                QString storedHash = query.value("password_hash").toString();
                QString inputHash = QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());

                if (storedHash == inputHash) {
                    // Успешная аутентификация
                    QString token = generateToken();
                    int userId = query.value("id").toInt();
                    QString role = query.value("role").toString();

                    // Сохранение токена в базе данных
                    QSqlQuery tokenQuery(db);
                    tokenQuery.prepare("INSERT INTO tokens (user_id, token, expires_at) VALUES (:user_id, :token, :expires_at)");
                    tokenQuery.bindValue(":user_id", userId);
                    tokenQuery.bindValue(":token", token);
                    tokenQuery.bindValue(":expires_at", QDateTime::currentDateTimeUtc().addSecs(3600)); // Токен действителен 1 час
                    if (!tokenQuery.exec()) {
                        responseObj["status"] = "error";
                        responseObj["message"] = "Ошибка при сохранении токена";
                    } else {
                        responseObj["status"] = "success";
                        responseObj["token"] = token;
                        responseObj["role"] = role;
                    }
                } else {
                    // Неверный пароль
                    responseObj["status"] = "error";
                    responseObj["message"] = "Неверные учетные данные";
                }
            } else {
                // Пользователь не найден
                responseObj["status"] = "error";
                responseObj["message"] = "Пользователь не найден";
            }

        } else {
            // Все остальные действия требуют проверки токена
            QString token = requestObj.value("token").toString();
            int userId;
            QString userRole;
            if (!validateToken(token, userId, userRole)) {
                responseObj["status"] = "error";
                responseObj["message"] = "Недействительный токен";
            } else if (action == "getData") {
                // Обработка запроса на получение данных
                QString dataType = requestObj.value("dataType").toString();
                // Проверка прав доступа может быть добавлена в зависимости от userRole и dataType
                responseObj["status"] = "success";
                responseObj["data"] = getDataFromDatabase(dataType, userId);

            } else if (action == "sendData") {
                // Обработка отправки данных
                QJsonObject dataToSend = requestObj.value("data").toObject();
                bool success = saveDataToDatabase(dataToSend, userId, userRole);

                if (success) {
                    responseObj["status"] = "success";
                } else {
                    responseObj["status"] = "error";
                    responseObj["message"] = "Не удалось сохранить данные";
                }
            } else {
                responseObj["status"] = "error";
                responseObj["message"] = "Неизвестное действие";
            }
        }

        QJsonDocument responseDoc(responseObj);
        socket.write(responseDoc.toJson(QJsonDocument::Compact));
        socket.flush();
    }

    socket.disconnectFromHost();
    socket.waitForDisconnected();
}

QString ClientHandler::generateToken()
{
    // Генерация случайного токена
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool ClientHandler::validateToken(const QString &token, int &userId, QString &userRole)
{
    QSqlQuery query(db);
    query.prepare("SELECT tokens.user_id, users.role, tokens.expires_at FROM tokens INNER JOIN users ON tokens.user_id = users.id WHERE tokens.token = :token");
    query.bindValue(":token", token);
    if (query.exec() && query.next()) {
        QDateTime expiresAt = query.value("expires_at").toDateTime();
        if (expiresAt > QDateTime::currentDateTimeUtc()) {
            userId = query.value("user_id").toInt();
            userRole = query.value("role").toString();
            return true;
        } else {
            // Токен истек, удалить его
            QSqlQuery deleteQuery(db);
            deleteQuery.prepare("DELETE FROM tokens WHERE token = :token");
            deleteQuery.bindValue(":token", token);
            deleteQuery.exec();
        }
    }
    return false;
}

QJsonArray ClientHandler::getDataFromDatabase(const QString &dataType, int userId)
{
    QJsonArray dataArray;
    QSqlQuery query(db);

    if (dataType == "rooms") {
        // Получаем комнаты, в которых состоит пользователь
        query.prepare("SELECT rooms.id, rooms.name FROM rooms INNER JOIN room_members ON rooms.id = room_members.room_id WHERE room_members.user_id = :user_id");
        query.bindValue(":user_id", userId);
        if (query.exec()) {
            while (query.next()) {
                QJsonObject room;
                room["id"] = query.value("id").toInt();
                room["name"] = query.value("name").toString();
                dataArray.append(room);
            }
        }
    } else if (dataType == "tasks") {
        // Получаем задачи, которые назначены пользователю или созданы пользователем
        query.prepare("SELECT id, title, description, status FROM tasks WHERE assignee_id = :user_id OR creator_id = :user_id");
        query.bindValue(":user_id", userId);
        if (query.exec()) {
            while (query.next()) {
                QJsonObject task;
                task["id"] = query.value("id").toInt();
                task["title"] = query.value("title").toString();
                task["description"] = query.value("description").toString();
                task["status"] = query.value("status").toString();
                dataArray.append(task);
            }
        }
    } else if (dataType == "messages") {
        // Получаем сообщения из комнат, в которых состоит пользователь
        query.prepare("SELECT messages.id, messages.room_id, messages.user_id, messages.content, messages.timestamp FROM messages INNER JOIN room_members ON messages.room_id = room_members.room_id WHERE room_members.user_id = :user_id");
        query.bindValue(":user_id", userId);
        if (query.exec()) {
            while (query.next()) {
                QJsonObject message;
                message["id"] = query.value("id").toInt();
                message["room_id"] = query.value("room_id").toInt();
                message["user_id"] = query.value("user_id").toInt();
                message["content"] = query.value("content").toString();
                message["timestamp"] = query.value("timestamp").toDateTime().toString(Qt::ISODate);
                dataArray.append(message);
            }
        }
    }

    return dataArray;
}

bool ClientHandler::saveDataToDatabase(const QJsonObject &data, int userId, const QString &userRole)
{
    QString dataType = data.value("type").toString();

    if (dataType == "message") {
        QSqlQuery query(db);
        // Проверить, что пользователь состоит в комнате
        int roomId = data.value("room_id").toInt();
        QSqlQuery checkQuery(db);
        checkQuery.prepare("SELECT 1 FROM room_members WHERE room_id = :room_id AND user_id = :user_id");
        checkQuery.bindValue(":room_id", roomId);
        checkQuery.bindValue(":user_id", userId);
        if (checkQuery.exec() && checkQuery.next()) {
            // Пользователь состоит в комнате, можно отправить сообщение
            query.prepare("INSERT INTO messages (room_id, user_id, content, timestamp) VALUES (:room_id, :user_id, :content, :timestamp)");
            query.bindValue(":room_id", roomId);
            query.bindValue(":user_id", userId);
            query.bindValue(":content", data.value("content").toString());
            query.bindValue(":timestamp", QDateTime::currentDateTimeUtc());
            return query.exec();
        } else {
            // Пользователь не состоит в комнате
            return false;
        }
    } else if (dataType == "task") {
        // Создание новой задачи
        QSqlQuery query(db);
        query.prepare("INSERT INTO tasks (title, description, status, assignee_id, creator_id, created_at, updated_at) VALUES (:title, :description, :status, :assignee_id, :creator_id, :created_at, :updated_at)");
        query.bindValue(":title", data.value("title").toString());
        query.bindValue(":description", data.value("description").toString());
        query.bindValue(":status", data.value("status").toString());
        query.bindValue(":assignee_id", data.value("assignee_id").toInt());
        query.bindValue(":creator_id", userId);
        query.bindValue(":created_at", QDateTime::currentDateTimeUtc());
        query.bindValue(":updated_at", QDateTime::currentDateTimeUtc());
        return query.exec();
    } else if (dataType == "update_task_status") {
        // Изменение статуса задачи
        int taskId = data.value("task_id").toInt();
        QString newStatus = data.value("status").toString();

        // Проверка, что пользователь имеет право изменять задачу
        // Допустим, что только исполнители или создатели задачи могут менять статус
        QSqlQuery checkQuery(db);
        checkQuery.prepare("SELECT 1 FROM tasks WHERE id = :task_id AND (assignee_id = :user_id OR creator_id = :user_id)");
        checkQuery.bindValue(":task_id", taskId);
        checkQuery.bindValue(":user_id", userId);
        if (checkQuery.exec() && checkQuery.next()) {
            QSqlQuery query(db);
            query.prepare("UPDATE tasks SET status = :status, updated_at = :updated_at WHERE id = :task_id");
            query.bindValue(":status", newStatus);
            query.bindValue(":updated_at", QDateTime::currentDateTimeUtc());
            query.bindValue(":task_id", taskId);
            return query.exec();
        } else {
            // Нет права изменять задачу
            return false;
        }
    }

    return false;
}