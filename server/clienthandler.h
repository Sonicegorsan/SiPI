/**
 * @file clienthandler.h
 * @brief Заголовочный файл класса обработчика клиентских соединений
 */

#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include <QSqlDatabase>

/**
 * @class ClientHandler
 * @brief Класс, обрабатывающий соединение с клиентом и обрабатывающий запросы
 * 
 * Класс ClientHandler отвечает за обработку отдельного клиентского подключения.
 * Он взаимодействует с базой данных, обрабатывает команды клиента и отправляет
 * ответы. Поддерживает авторизацию, регистрацию, работу с сообщениями, задачами и отчетами.
 */
class ClientHandler : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Конструктор класса ClientHandler
     * @param socketDescriptor Дескриптор сокета нового подключения
     * @param db Соединение с базой данных
     * @param parent Родительский объект
     */
    explicit ClientHandler(qintptr socketDescriptor, QSqlDatabase db, QObject *parent = nullptr);

signals:
    /**
     * @brief Сигнал, отправляемый при отключении клиента
     */
    void disconnected();

private slots:
    /**
     * @brief Слот для обработки данных, поступающих от клиента
     */
    void onReadyRead();
    
    /**
     * @brief Слот для обработки отключения клиента
     */
    void onDisconnected();

private:
    QTcpSocket *socket; ///< Сокет соединения с клиентом
    QSqlDatabase db; ///< Соединение с базой данных
    QString username; ///< Имя пользователя после авторизации

    /**
     * @brief Обрабатывает полученные от клиента данные
     * @param data Данные в формате JSON
     */
    void processData(const QByteArray &data);
    
    /**
     * @brief Отправляет ответ клиенту
     * @param data Данные в формате JSON
     */
    void sendResponse(const QByteArray &data);
};

#endif // CLIENTHANDLER_H
