#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QTcpSocket>

class QLineEdit;
class QPushButton;
class QLabel;

class LoginWindow : public QWidget {
    Q_OBJECT
public:
    explicit LoginWindow(QTcpSocket *socket, QWidget *parent = nullptr);

signals:
    void loginSuccess(const QString &username, const QString &role);

private slots:
    void onLoginButtonClicked();
    void onRegisterButtonClicked();
    void onReadyRead();
    void onConnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    void setupUI();

    QLineEdit *usernameLineEdit;
    QLineEdit *passwordLineEdit;
    QPushButton *loginButton;
    QPushButton *registerButton;
    QLabel *statusLabel;

    QTcpSocket *socket;

    enum RequestType { LoginRequest, RegisterRequest };
    RequestType currentRequest;
};

#endif // LOGINWINDOW_H
