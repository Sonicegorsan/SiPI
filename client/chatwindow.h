#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QWidget>
#include <QTcpSocket>
#include <QTimer>

class QTextEdit;
class QLineEdit;
class QPushButton;

class ChatWindow : public QWidget {
    Q_OBJECT
public:
    explicit ChatWindow(int roomId, const QString &roomName, QTcpSocket *socket, QWidget *parent = nullptr);

private slots:
    void onSendButtonClicked();
    void onReadyRead();
    void requestMessages();

private:
    void setupUI();

    int roomId;
    QString roomName;
    QTcpSocket *socket;

    QTextEdit *messageView;
    QLineEdit *messageInput;
    QPushButton *sendButton;
    QTimer *updateTimer;
};

#endif // CHATWINDOW_H
