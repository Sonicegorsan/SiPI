#include "mainwindow.h"
#include "chatwindow.h"
#include <QTreeWidget>
#include <QCalendarWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QSplitter>
#include <QTreeWidgetItem>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QTextCharFormat>

MainWindow::MainWindow(const QString &username, const QString &role, QTcpSocket *socket, QWidget *parent)
    : QMainWindow(parent), username(username), role(role), socket(socket) {
    setupUI();
    requestRooms();

    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    chatTree = new QTreeWidget();
    chatTree->setHeaderLabel("Чаты");
    connect(chatTree, &QTreeWidget::itemClicked, this, &MainWindow::onChatItemClicked);

    calendar = new QCalendarWidget();

    welcomeLabel = new QLabel(QString("Добро пожаловать, %1 (%2)").arg(username, role));

    QSplitter *splitter = new QSplitter();
    splitter->addWidget(chatTree);
    splitter->addWidget(calendar);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->addWidget(welcomeLabel);
    mainLayout->addWidget(splitter);

    centralWidget->setLayout(mainLayout);

    setWindowTitle("ВСРИРП - Главное окно");

    // Подключение сигнала смены месяца
    connect(calendar, &QCalendarWidget::currentPageChanged, this, [&](int year, int month){
        QDate startDate(year, month, 1);
        QDate endDate = startDate.addMonths(1).addDays(-1);
        requestTasks(startDate, endDate);
    });

    // Запрос задач на текущий месяц
    QDate currentDate = QDate::currentDate();
    QDate startDate(currentDate.year(), currentDate.month(), 1);
    QDate endDate = startDate.addMonths(1).addDays(-1);
    requestTasks(startDate, endDate);

    // Подключение сигнала выбора даты
    connect(calendar, &QCalendarWidget::activated, this, [&](const QDate &date){
        // Отображаем список задач на выбранную дату
        // Реализуйте функцию отображения задач
        qDebug() << "Выбрана дата:" << date.toString();
    });
}

void MainWindow::requestRooms() {
    // Отправляем запрос на сервер для получения списка комнат
    QJsonObject request;
    request["command"] = "GET_ROOMS";
    QJsonDocument doc(request);
    QByteArray data = doc.toJson();
    socket->write(data);
    socket->flush();
}

void MainWindow::requestTasks(const QDate &startDate, const QDate &endDate) {
    QJsonObject request;
    request["command"] = "GET_TASKS";
    request["start_date"] = startDate.toString(Qt::ISODate);
    request["end_date"] = endDate.toString(Qt::ISODate);

    QJsonDocument doc(request);
    QByteArray data = doc.toJson();
    socket->write(data);
    socket->flush();
}

void MainWindow::onReadyRead() {
    QByteArray responseData = socket->readAll();
    QJsonDocument responseDoc = QJsonDocument::fromJson(responseData);
    QJsonObject response = responseDoc.object();

    QString status = response["status"].toString();

    if (status == "OK") {
        if (response.contains("rooms")) {
            // Обработка списка комнат
            QJsonArray roomsArray = response["rooms"].toArray();
            chatTree->clear();
            for (const QJsonValue &value : roomsArray) {
                QJsonObject roomObj = value.toObject();
                QTreeWidgetItem *roomItem = new QTreeWidgetItem(chatTree);
                roomItem->setText(0, roomObj["name"].toString());
                roomItem->setData(0, Qt::UserRole, roomObj["id"].toInt());
            }
        } else if (response.contains("tasks")) {
            // Обработка списка задач
            QJsonArray tasksArray = response["tasks"].toArray();
            onTasksReceived(tasksArray);
        }
    } else {
        QString message = response["message"].toString();
        qWarning() << "Ошибка от сервера:" << message;
    }
}

void MainWindow::onTasksReceived(const QJsonArray &tasksArray) {
    // Очистить предыдущие заметки на календаре
    calendar->setDateTextFormat(QDate(), QTextCharFormat());

    // Обход полученных задач
    for (const QJsonValue &value : tasksArray) {
        QJsonObject taskObj = value.toObject();
        QString deadlineStr = taskObj["deadline"].toString();
        QDate deadline = QDate::fromString(deadlineStr, Qt::ISODate);

        QTextCharFormat format = calendar->dateTextFormat(deadline);
        format.setBackground(Qt::yellow); // Помечаем даты с задачами
        calendar->setDateTextFormat(deadline, format);
    }
}

void MainWindow::onChatItemClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column);
    if (item) {
        QString roomName = item->text(0);
        int roomId = item->data(0, Qt::UserRole).toInt();
        qDebug() << "Выбран чат:" << roomName << "ID:" << roomId;

        // Открываем окно чата
        ChatWindow *chatWindow = new ChatWindow(roomId, roomName, socket);
        chatWindow->show();
    }
}
