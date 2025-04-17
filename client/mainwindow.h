#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTreeWidgetItem>

class QTreeWidget;
class QCalendarWidget;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString &username, const QString &role, QTcpSocket *socket, QWidget *parent = nullptr);

private slots:
    void onChatItemClicked(QTreeWidgetItem *item, int column);
    void onReadyRead();

private:
    void setupUI();
    void requestRooms();
    void requestTasks(const QDate &startDate, const QDate &endDate);
    void onTasksReceived(const QJsonArray &tasksArray);

    QTreeWidget *chatTree;
    QCalendarWidget *calendar;
    QLabel *welcomeLabel;
    QString username;
    QString role;

    QTcpSocket *socket;
};

#endif // MAINWINDOW_H
