/**
 * @file server.h
 * @brief Заголовочный файл класса сервера, отвечающего за сетевое взаимодействие
 */

#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QSqlDatabase>

/**
 * @class Server
 * @brief Класс, реализующий TCP сервер для обработки подключений клиентов
 * 
 * Класс Server наследуется от QTcpServer и отвечает за прослушивание входящих
 * соединений и создание обработчиков клиентов. Также класс инициализирует
 * соединение с базой данных PostgreSQL.
 */
class Server : public QTcpServer {
    Q_OBJECT
public:
    /**
     * @brief Конструктор класса Server
     * @param parent Родительский объект
     */
    explicit Server(QObject *parent = nullptr);
    
    /**
     * @brief Деструктор класса Server
     */
    ~Server();

    /**
     * @brief Запуск сервера на указанном порту
     * @param port Номер порта для прослушивания
     * @return true при успешном запуске, false в случае ошибки
     */
    bool startServer(quint16 port);

protected:
    /**
     * @brief Переопределенный метод для обработки входящих соединений
     * @param socketDescriptor Дескриптор сокета нового подключения
     */
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QSqlDatabase db; ///< Объект соединения с базой данных
    
    /**
     * @brief Инициализация соединения с базой данных
     * @return true при успешном подключении, false в случае ошибки
     */
    bool initDatabase();
};

#endif // SERVER_H
