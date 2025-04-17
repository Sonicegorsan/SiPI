#include "loginwindow.h"
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>

LoginWindow::LoginWindow(QTcpSocket *socket, QWidget *parent)
    : QWidget(parent), socket(socket) {
    setupUI();
    connect(socket, &QTcpSocket::readyRead, this, &LoginWindow::onReadyRead);
    connect(socket, &QTcpSocket::connected, this, &LoginWindow::onConnected);

    // используем сигнал 'error', если 'errorOccurred' недоступен
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &LoginWindow::onErrorOccurred);
}

void LoginWindow::setupUI() {
    setWindowTitle("Вход в систему");

    QLabel *usernameLabel = new QLabel("Имя пользователя:");
    QLabel *passwordLabel = new QLabel("Пароль:");

    usernameLineEdit = new QLineEdit();
    passwordLineEdit = new QLineEdit();
    passwordLineEdit->setEchoMode(QLineEdit::Password);

    loginButton = new QPushButton("Войти");
    registerButton = new QPushButton("Зарегистрироваться");
    statusLabel = new QLabel();

    QHBoxLayout *usernameLayout = new QHBoxLayout();
    usernameLayout->addWidget(usernameLabel);
    usernameLayout->addWidget(usernameLineEdit);

    QHBoxLayout *passwordLayout = new QHBoxLayout();
    passwordLayout->addWidget(passwordLabel);
    passwordLayout->addWidget(passwordLineEdit);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(loginButton);
    buttonsLayout->addWidget(registerButton);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(usernameLayout);
    mainLayout->addLayout(passwordLayout);
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addWidget(statusLabel);

    setLayout(mainLayout);

    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::onLoginButtonClicked);
    connect(registerButton, &QPushButton::clicked, this, &LoginWindow::onRegisterButtonClicked);
}

void LoginWindow::onConnected() {
    qDebug() << "Подключено к серверу";
}

void LoginWindow::onErrorOccurred(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    statusLabel->setText("Ошибка подключения к серверу");
    loginButton->setEnabled(false);
    registerButton->setEnabled(false);
}

void LoginWindow::onLoginButtonClicked() {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        statusLabel->setText("Нет подключения к серверу");
        return;
    }

    QString username = usernameLineEdit->text();
    QString password = passwordLineEdit->text();

    // Хешируем пароль
    QByteArray passwordHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();

    // Создаём JSON-запрос
    QJsonObject request;
    request["command"] = "LOGIN";
    request["username"] = username;
    request["password"] = QString(passwordHash);

    QJsonDocument doc(request);
    QByteArray data = doc.toJson();

    socket->write(data);
    socket->flush();

    currentRequest = LoginRequest;
}

void LoginWindow::onRegisterButtonClicked() {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        statusLabel->setText("Нет подключения к серверу");
        return;
    }

    QString username = usernameLineEdit->text();
    QString password = passwordLineEdit->text();

    if (username.isEmpty() || password.isEmpty()) {
        statusLabel->setText("Введите имя пользователя и пароль");
        return;
    }

    // Хешируем пароль
    QByteArray passwordHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();

    // Создаём JSON-запрос
    QJsonObject request;
    request["command"] = "REGISTER";
    request["username"] = username;
    request["password"] = QString(passwordHash);

    QJsonDocument doc(request);
    QByteArray data = doc.toJson();

    socket->write(data);
    socket->flush();

    currentRequest = RegisterRequest;
}

void LoginWindow::onReadyRead() {
    QByteArray responseData = socket->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    QJsonObject response = responseDoc.object();

    QString status = response["status"].toString();
    QString message = response["message"].toString();

    if (status == "OK") {
        if (currentRequest == LoginRequest) {
            QString role = response["role"].toString();
            emit loginSuccess(usernameLineEdit->text(), role);
            this->close();
        } else if (currentRequest == RegisterRequest) {
            statusLabel->setText("Регистрация успешна. Теперь вы можете войти.");
        }
    } else {
        statusLabel->setText(message);
    }
}
