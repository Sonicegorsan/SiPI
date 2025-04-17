/**
 * @file loginwindow.h
 * @brief Заголовочный файл класса окна авторизации
 */

#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QTcpSocket>

class QLineEdit;
class QPushButton;
class QLabel;

/**
 * @class LoginWindow
 * @brief Класс, реализующий окно авторизации и регистрации пользователей
 * 
 * LoginWindow предоставляет интерфейс для входа существующих пользователей и
 * регистрации новых. Содержит поля для ввода имени пользователя и пароля, а также
 * кнопки для выполнения авторизации и регистрации.
 */
class LoginWindow : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Конструктор класса LoginWindow
     * @param socket Сокет для соединения с сервером
     * @param parent Родительский виджет
     */
    explicit LoginWindow(QTcpSocket *socket, QWidget *parent = nullptr);

signals:
    /**
     * @brief Сигнал, отправляемый при успешной авторизации
     * @param username Имя пользователя
     * @param role Роль пользователя
     */
    void loginSuccess(const QString &username, const QString &role);

private slots:
    /**
     * @brief Слот, вызываемый при нажатии кнопки входа
     */
    void onLoginButtonClicked();
    
    /**
     * @brief Слот, вызываемый при нажатии кнопки регистрации
     */
    void onRegisterButtonClicked();
    
    /**
     * @brief Слот для обработки данных, поступающих от сервера
     */
    void onReadyRead();
    
    /**
     * @brief Слот, вызываемый при установлении соединения с сервером
     */
    void onConnected();
    
    /**
     * @brief Слот для обработки ошибок соединения
     * @param socketError Код ошибки сокета
     */
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    /**
     * @brief Настраивает пользовательский интерфейс окна авторизации
     */
    void setupUI();

    QLineEdit *usernameLineEdit;    ///< Поле для ввода имени пользователя
    QLineEdit *passwordLineEdit;    ///< Поле для ввода пароля
    QPushButton *loginButton;       ///< Кнопка входа
    QPushButton *registerButton;    ///< Кнопка регистрации
    QLabel *statusLabel;            ///< Метка для вывода статусных сообщений

    QTcpSocket *socket;             ///< Сокет для связи с сервером

    /**
     * @brief Типы запросов, которые могут быть отправлены
     */
    enum RequestType { LoginRequest, RegisterRequest };
    RequestType currentRequest;     ///< Текущий тип запроса
};

#endif // LOGINWINDOW_H
