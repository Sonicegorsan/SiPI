/**
 * @file mainwindow.h
 * @brief Заголовочный файл класса главного окна приложения
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTreeWidgetItem>

class QTreeWidget;
class QCalendarWidget;
class QLabel;

/**
 * @class MainWindow
 * @brief Класс главного окна приложения
 * 
 * MainWindow предоставляет основной интерфейс приложения после авторизации.
 * Отображает список доступных комнат, календарь с задачами и предоставляет
 * доступ к функциям, соответствующим роли пользователя.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    /**
     * @brief Конструктор класса MainWindow
     * @param username Имя пользователя
     * @param role Роль пользователя
     * @param socket Сокет для соединения с сервером
     * @param parent Родительский виджет
     */
    explicit MainWindow(const QString &username, const QString &role, QTcpSocket *socket, QWidget *parent = nullptr);

private slots:
    /**
     * @brief Слот, вызываемый при выборе элемента в дереве чатов
     * @param item Выбранный элемент
     * @param column Выбранная колонка
     */
    void onChatItemClicked(QTreeWidgetItem *item, int column);
    
    /**
     * @brief Слот для обработки данных, поступающих от сервера
     */
    void onReadyRead();

private:
    /**
     * @brief Настраивает пользовательский интерфейс главного окна
     */
    void setupUI();
    
    /**
     * @brief Запрашивает список доступных комнат с сервера
     */
    void requestRooms();
    
    /**
     * @brief Запрашивает задачи за указанный период
     * @param startDate Начальная дата периода
     * @param endDate Конечная дата периода
     */
    void requestTasks(const QDate &startDate, const QDate &endDate);
    
    /**
     * @brief Обрабатывает полученные с сервера задачи
     * @param tasksArray Массив задач в формате JSON
     */
    void onTasksReceived(const QJsonArray &tasksArray);

    QTreeWidget *chatTree;          ///< Виджет для отображения дерева комнат
    QCalendarWidget *calendar;      ///< Виджет календаря для отображения и выбора дат
    QLabel *welcomeLabel;           ///< Метка с приветствием пользователя
    QString username;               ///< Имя аутентифицированного пользователя
    QString role;                   ///< Роль пользователя

    QTcpSocket *socket;             ///< Сокет для связи с сервером
};

#endif // MAINWINDOW_H
