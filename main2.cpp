#include <QApplication>
#include "loginwindow.h"
#include "mainwindow.h" // Обязательно включите этот заголовок

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    QTcpSocket *socket = new QTcpSocket();
    socket->connectToHost("127.0.0.1", 12345);

    LoginWindow loginWindow(socket);
    loginWindow.show();

    QObject::connect(&loginWindow, &LoginWindow::loginSuccess, [&](const QString &username, const QString &role){
        MainWindow *mainWindow = new MainWindow(username, role, socket);
        mainWindow->show();
    });

    return a.exec();
}
