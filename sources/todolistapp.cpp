#include "../headers/todolistapp.h"
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QStandardPaths>
#include <QDir>

ToDoListApp::ToDoListApp(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("To-Do List App");

    QVBoxLayout *layout = new QVBoxLayout;

    taskInput = new QLineEdit;
    addButton = new QPushButton("Add Task");
    taskList = new QListWidget;
    saveButton = new QPushButton("Save Tasks");
    loadButton = new QPushButton("Load Tasks");
    addImageButton = new QPushButton("Add Image");
    imageLabel = new QLabel;

    layout->addWidget(taskInput);
    layout->addWidget(addButton);
    layout->addWidget(taskList);
    layout->addWidget(saveButton);
    layout->addWidget(loadButton);
    layout->addWidget(addImageButton);
    layout->addWidget(imageLabel);

    QWidget *central = new QWidget(this);
    central->setLayout(layout);
    setCentralWidget(central);

    connect(addButton, &QPushButton::clicked, this, &ToDoListApp::addTask);
    connect(taskList, &QListWidget::itemDoubleClicked, this, &ToDoListApp::toggleTaskComplete);
    connect(saveButton, &QPushButton::clicked, this, &ToDoListApp::saveTasks);
    connect(loadButton, &QPushButton::clicked, this, &ToDoListApp::loadTasks);
    connect(addImageButton, &QPushButton::clicked, this, &ToDoListApp::addImageToTask);
    connect(taskList, &QListWidget::itemChanged,
        this, &ToDoListApp::onItemChanged);
    
    QString docsDir = QStandardPaths::writableLocation(
                        QStandardPaths::DocumentsLocation);
    if (docsDir.isEmpty())
        docsDir = QDir::homePath();

    const QString cacheDir = docsDir + "/.todo-list";
    QDir().mkpath(cacheDir);
    cacheFilePath = cacheDir + "/cached_tasks.json";

    cacheTasksFromCacheFile();
}

void ToDoListApp::addTask() {
    QString taskDescription = taskInput->text();
    if (!taskDescription.isEmpty()) {
        Task task(taskDescription);
        tasks.append(task);
        updateTaskList();
        taskInput->clear();
        cacheTasksToFile();
    }
}

Task* ToDoListApp::findTaskById(int id) {
    for (Task &t : tasks)
        if (t.getId() == id)
            return &t;
    return nullptr;
}

void ToDoListApp::toggleTaskComplete(QListWidgetItem *item) {
    if (!item) return;
    const int id = item->data(Qt::UserRole).toInt();
    if (Task *t = findTaskById(id)) {
        t->toggleComplete();
        updateTaskList();
        cacheTasksToFile();
    }
}

void ToDoListApp::saveTasks() {
    QFile file("tasks.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        for (const Task &task : tasks) {
            stream << task.getDescription() << "\t" << (task.isCompleted() ? "1" : "0") << "\t" << task.getImagePath() << "\n";
        }
        file.close();
        QMessageBox::information(this, "Tasks Saved", "Tasks saved to tasks.txt");
    } else {
        QMessageBox::warning(this, "Error", "Could not save tasks to file.");
    }
    cacheTasksToFile();
}

void ToDoListApp::loadTasks() {
    taskList->clear();
    tasks.clear();
    QFile file("tasks.txt");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            QStringList parts = line.split('\t');
            if (parts.size() >= 3) {
                Task task(parts[0], parts[1] == "1");
                task.setImagePath(parts[2]);
                tasks.append(task);
            }
        }
        file.close();
        updateTaskList();
        QMessageBox::information(this, "Tasks Loaded", "Tasks loaded from tasks.txt");
    } else {
        QMessageBox::warning(this, "Error", "Could not load tasks from file.");
    }
    cacheTasksToFile();
}

void ToDoListApp::addImageToTask() {
    QListWidgetItem *item = taskList->currentItem();
    if (!item) return;

    const int id = item->data(Qt::UserRole).toInt();
    Task *t = findTaskById(id);
    if (!t) return;

    const QString imagePath = QFileDialog::getOpenFileName(
        this, "Select Image", "", "Images (*.png *.jpg *.jpeg)");
    if (imagePath.isEmpty()) return;

    t->setImagePath(imagePath);
    cacheTasksToFile();
}

void ToDoListApp::updateTaskList() {
    updatingList = true;
    taskList->clear();
    for (const Task &task : tasks) {
        auto *item = new QListWidgetItem(task.getDescription());
        item->setData(Qt::UserRole, task.getId());
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(task.isCompleted() ? Qt::Checked : Qt::Unchecked);
        taskList->addItem(item);
    }
    updatingList = false;
}

void ToDoListApp::cacheTasksToFile() {
    QFile cacheFile(cacheFilePath);
    if (cacheFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QJsonArray tasksArray;
        for (const Task &task : tasks) {
            QJsonObject taskObject;
            taskObject["description"] = task.getDescription();
            taskObject["completed"] = task.isCompleted();
            taskObject["imagePath"] = task.getImagePath();
            tasksArray.append(taskObject);
        }

        QJsonDocument jsonDocument(tasksArray);
        QTextStream stream(&cacheFile);
        stream << jsonDocument.toJson(QJsonDocument::Indented);
        cacheFile.close();
    }
}

void ToDoListApp::cacheTasksFromCacheFile() {
    QFile cacheFile(cacheFilePath);
    if (cacheFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray jsonData = cacheFile.readAll();
        QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData);
        if (jsonDocument.isArray()) {
            QJsonArray tasksArray = jsonDocument.array();
            tasks.clear();
            for (const QJsonValue &taskValue : tasksArray) {
                if (taskValue.isObject()) {
                    QJsonObject taskObject = taskValue.toObject();
                    QString description = taskObject["description"].toString();
                    bool completed = taskObject["completed"].toBool();
                    QString imagePath = taskObject["imagePath"].toString();
                    Task task(description, completed);
                    task.setImagePath(imagePath);
                    tasks.append(task);
                }
            }
            updateTaskList();
        }
        cacheFile.close();
    }
}

void ToDoListApp::onItemChanged(QListWidgetItem *item) {
    if (updatingList) return;
    if (!item) return;

    const int id = item->data(Qt::UserRole).toInt();
    Task *t = findTaskById(id);
    if (!t) return;

    const bool checked = (item->checkState() == Qt::Checked);
    if (t->isCompleted() != checked) {
        t->toggleComplete();
        cacheTasksToFile();
    }
}