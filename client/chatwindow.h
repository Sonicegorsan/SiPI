/**
 * @file chatwindow.h
 * @brief Заголовочный файл класса окна чата
 */

#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QTcpSocket>
#include <QTimer>

class QTextEdit;
class QLineEdit;
class QPushButton;

/**
 * @class ChatWindow
 * @brief Класс, реализующий окно чата для отправки и получения сообщений
 * 
 * ChatWindow предоставляет интерфейс для просмотра истории сообщений в комнате,
 * отправки новых сообщений и автоматического обновления содержимого чата.
 */
class ChatWindow : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Конструктор класса ChatWindow
     * @param roomId Идентификатор комнаты
     * @param roomName Название комнаты
     * @param socket Сокет для соединения с сервером
     * @param parent Родительский виджет
     */
    explicit ChatWindow(int roomId, const QString &roomName, QTcpSocket *socket, QWidget *parent = nullptr);

private slots:
    /**
     * @brief Слот, вызываемый при нажатии кнопки отправки сообщения
     */
    void onSendButtonClicked();
    
    /**
     * @brief Слот для обработки данных, поступающих от сервера
     */
    void onReadyRead();
    
    /**
     * @brief Запрашивает сообщения от сервера
     * 
     * Вызывается периодически по таймеру для обновления сообщений
     */
    void requestMessages();

private:
    /**
     * @brief Настраивает пользовательский интерфейс окна чата
     */
    void setupUI();

    int roomId;                     ///< Идентификатор текущей комнаты
    QString roomName;               ///< Название текущей комнаты
    QTcpSocket *socket;             ///< Сокет для связи с сервером

    QTextEdit *messageView;         ///< Виджет для отображения сообщений
    QLineEdit *messageInput;        ///< Поле для ввода нового сообщения
    QPushButton *sendButton;        ///< Кнопка отправки сообщения
    QTimer *updateTimer;            ///< Таймер для периодического обновления сообщений
};

#endif // CHATWINDOW_H
