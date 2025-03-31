// main.cpp
// Главный файл приложения клиент-сервер на Qt, с использованием заголовочных файлов и PostgreSQL в качестве базы данных

#include <QCoreApplication>
#include "server.h"
#include "client.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Запуск сервера
    Server server;

    // Проверка успешного запуска сервера
    if (!server.isListening()) {
        qCritical() << "Сервер не может быть запущен.";
        return -1;
    }

    // Создание клиента
    Client client;

    return a.exec();
}