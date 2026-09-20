#ifndef TODOLISTAPP_H
#define TODOLISTAPP_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include "task.h"

class ToDoListApp : public QMainWindow {
    Q_OBJECT
public:
    ToDoListApp(QWidget *parent = nullptr);

private slots:
    void addTask();
    void toggleTaskComplete(QListWidgetItem *item);
    void onItemChanged(QListWidgetItem *item);
    void onSelectionChanged();
    void onContextMenuRequested(const QPoint &pos);
    void editTask();
    void saveTasks();
    void loadTasks();
    void addImageToTask();

private:
    void updateTaskList();
    void updateImagePreview(const Task &task);
    void cacheTasksToFile();
    void cacheTasksFromCacheFile();
    Task* findTaskById(int id);

    bool isPathSafeForWrite(const QString &path, QString &reason) const;
    bool writeTasksToFile(const QString &path, QString &error) const;
    bool readTasksFromFile(const QString &path, QVector<Task> &out, QString &error) const;

    QString readLastPath(const QString &filePath, const QString &fallback) const;
    void writeLastPath(const QString &filePath, const QString &value) const;

    QLineEdit *taskInput;
    QPushButton *addButton;
    QListWidget *taskList;
    QPushButton *saveButton;
    QPushButton *loadButton;
    QPushButton *addImageButton;
    QLabel *imageLabel;
    QVector<Task> tasks;
    QString cacheFilePath;
    QString pathsDir;
    bool updatingList = false;
};

#endif // TODOLISTAPP_H