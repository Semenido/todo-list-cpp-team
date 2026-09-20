#include "../headers/edittaskdialog.h"
#include <QMenu>
#include <QPoint>
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
#include <QFileInfo>

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

    imageLabel->setText("(no task selected)");
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumHeight(100);

    connect(addButton, &QPushButton::clicked, this, &ToDoListApp::addTask);
    connect(taskList, &QListWidget::itemDoubleClicked, this, &ToDoListApp::toggleTaskComplete);
    connect(taskList, &QListWidget::itemChanged, this, &ToDoListApp::onItemChanged);
    connect(taskList, &QListWidget::itemSelectionChanged, this, &ToDoListApp::onSelectionChanged);
    connect(saveButton, &QPushButton::clicked, this, &ToDoListApp::saveTasks);
    connect(loadButton, &QPushButton::clicked, this, &ToDoListApp::loadTasks);
    connect(addImageButton, &QPushButton::clicked, this, &ToDoListApp::addImageToTask);
            taskList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(taskList, &QListWidget::customContextMenuRequested,
            this, &ToDoListApp::onContextMenuRequested);
    
    QString docsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docsDir.isEmpty())
        docsDir = QDir::homePath();

    const QString baseDir = docsDir + "/.todo-list";
    pathsDir = baseDir + "/paths";

    QDir().mkpath(pathsDir);
    cacheFilePath = baseDir + "/cached_tasks.json";

    cacheTasksFromCacheFile();
}

void ToDoListApp::addTask() {
    const QString taskDescription = taskInput->text().trimmed();
    if (taskDescription.isEmpty())
        return;

    tasks.append(Task(taskDescription));
    updateTaskList();
    taskInput->clear();
    cacheTasksToFile();
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

void ToDoListApp::onSelectionChanged() {
    QListWidgetItem *item = taskList->currentItem();
    if (!item) {
        imageLabel->clear();
        imageLabel->setText("(no task selected)");
        return;
    }
    const int id = item->data(Qt::UserRole).toInt();
    if (Task *t = findTaskById(id))
        updateImagePreview(*t);
}

void ToDoListApp::updateTaskList() {
    updatingList = true;
    taskList->clear();
    for (const Task &task : tasks) {
        auto *item = new QListWidgetItem(task.getDescription());
        item->setData(Qt::UserRole, task.getId());

        if (!task.getComment().isEmpty()) {
            item->setToolTip(task.getComment());
        }

        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(task.isCompleted() ? Qt::Checked : Qt::Unchecked);
        taskList->addItem(item);
    }
    updatingList = false;

    if (taskList->currentItem())
        onSelectionChanged();
    else {
        imageLabel->clear();
        imageLabel->setText("(no task selected)");
    }
}

void ToDoListApp::updateImagePreview(const Task &task) {
    const QString path = task.getImagePath();
    if (path.isEmpty()) {
        imageLabel->clear();
        imageLabel->setText("(no image)");
        return;
    }
    QPixmap pix(path);
    if (pix.isNull()) {
        imageLabel->clear();
        imageLabel->setText("(image not found)");
        return;
    }
    imageLabel->setPixmap(pix.scaledToHeight(100, Qt::SmoothTransformation));
}

void ToDoListApp::addImageToTask() {
    QListWidgetItem *item = taskList->currentItem();
    if (!item) {
        QMessageBox::information(this, "Add Image",
                                 "Сначала выберите задачу в списке.");
        return;
    }

    const int id = item->data(Qt::UserRole).toInt();
    Task *t = findTaskById(id);
    if (!t) return;

    const QString imagePath = QFileDialog::getOpenFileName(
        this, "Select Image", "", "Images (*.png *.jpg *.jpeg)");
    if (imagePath.isEmpty()) return;

    t->setImagePath(imagePath);
    updateImagePreview(*t);
    cacheTasksToFile();
}

// context menu

void ToDoListApp::onContextMenuRequested(const QPoint &pos) {
    QListWidgetItem *item = taskList->itemAt(pos);
    if (!item)
        return;

    taskList->setCurrentItem(item);

    QMenu menu(this);
    QAction *editAction = menu.addAction("Редактировать");
    QAction *chosen = menu.exec(taskList->viewport()->mapToGlobal(pos));

    if (chosen == editAction)
        editTask();
}

void ToDoListApp::editTask() {
    QListWidgetItem *item = taskList->currentItem();
    if (!item) {
        QMessageBox::information(this, "Edit Task",
                                 "Сначала выберите задачу в списке.");
        return;
    }

    const int id = item->data(Qt::UserRole).toInt();
    Task *t = findTaskById(id);
    if (!t)
        return;

    EditTaskDialog dialog(this);
    dialog.setDescription(t->getDescription());
    dialog.setComment(t->getComment());

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString newDescription = dialog.description();
    const QString newComment = dialog.comment();

    if (newDescription == t->getDescription() && newComment == t->getComment())
        return;

    t->setDescription(newDescription);
    t->setComment(newComment);

    updateTaskList();
    cacheTasksToFile();
}

// last path

QString ToDoListApp::readLastPath(const QString &filePath,
                                  const QString &fallback) const {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return fallback;

    QTextStream stream(&f);
    const QString line = stream.readLine().trimmed();
    f.close();

    if (line.isEmpty())
        return fallback;

    const QFileInfo info(line);
    if (info.exists())
        return info.absoluteFilePath();

    const QFileInfo dirInfo(info.absolutePath());
    if (dirInfo.exists() && dirInfo.isDir())
        return dirInfo.absoluteFilePath();

    return fallback;
}

void ToDoListApp::writeLastPath(const QString &filePath,
                                const QString &value) const {
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    QTextStream stream(&f);
    stream << value << '\n';
    f.close();
}

// Save / Load

bool ToDoListApp::isPathSafeForWrite(const QString &path, QString &reason) const {
    if (path.isEmpty()) {
        reason = "Путь не задан.";
        return false;
    }

    const QFileInfo info(path);
    const QString abs = info.absoluteFilePath();

    static const QStringList forbiddenRoots = {
        "/", "/bin", "/boot", "/dev", "/etc", "/lib", "/lib64",
        "/proc", "/root", "/sbin", "/sys", "/usr", "/var"
    };
    for (const QString &root : forbiddenRoots) {
        if (abs == root || abs.startsWith(root + "/")) {
            reason = QString("Запись в системную директорию «%1» запрещена.").arg(root);
            return false;
        }
    }

    const QFileInfo dirInfo(info.absolutePath());
    if (!dirInfo.exists() || !dirInfo.isDir()) {
        reason = "Каталог назначения не существует.";
        return false;
    }
    if (!dirInfo.isWritable()) {
        reason = "Нет прав на запись в каталог назначения.";
        return false;
    }

    if (info.exists() && !info.isWritable()) {
        reason = "Файл существует и защищён от записи.";
        return false;
    }

    return true;
}

bool ToDoListApp::writeTasksToFile(const QString &path, QString &error) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        error = file.errorString();
        return false;
    }

    QTextStream stream(&file);
    for (const Task &task : tasks) {
        QString desc = task.getDescription();
        desc.replace('\\', "\\\\");
        desc.replace('\t', "\\t");
        desc.replace('\n', "\\n");

        QString img = task.getImagePath();
        img.replace('\\', "\\\\");
        img.replace('\t', "\\t");
        img.replace('\n', "\\n");

        stream << desc << '\t'
               << (task.isCompleted() ? '1' : '0') << '\t'
               << img << '\n';
    }

    stream.flush();
    if (file.error() != QFileDevice::NoError) {
        error = file.errorString();
        file.close();
        return false;
    }
    file.close();
    return true;
}

bool ToDoListApp::readTasksFromFile(const QString &path,
                                    QVector<Task> &out,
                                    QString &error) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = file.errorString();
        return false;
    }

    auto unescape = [](QString s) {
        s.replace("\\n", "\n");
        s.replace("\\t", "\t");
        s.replace("\\\\", "\\");
        return s;
    };

    QVector<Task> loaded;
    QTextStream stream(&file);
    int lineNo = 0;
    while (!stream.atEnd()) {
        ++lineNo;
        const QString line = stream.readLine();
        if (line.isEmpty())
            continue;

        const QStringList parts = line.split('\t');
        if (parts.size() < 3) {
            error = QString("Ошибка: выбранный файл не является файлом сохранения todo-list-app, либо был поврежден или изменен.")
                        .arg(lineNo).arg(parts.size());
            file.close();
            return false;
        }

        Task task(unescape(parts[0]), parts[1] == "1");
        task.setImagePath(unescape(parts[2]));
        loaded.append(task);
    }

    if (file.error() != QFileDevice::NoError) {
        error = file.errorString();
        file.close();
        return false;
    }

    file.close();
    out = loaded;
    return true;
}

void ToDoListApp::saveTasks() {
    const QString lastSaveFile = pathsDir + "/last_save_path.txt";
    const QString startPath = readLastPath(lastSaveFile, QDir::homePath());

    QString selected = QFileDialog::getSaveFileName(
        this,
        "Save Tasks As",
        startPath,
        "Text files (*.txt);;All files (*)");
    if (selected.isEmpty())
        return;

    if (!selected.endsWith(".txt", Qt::CaseInsensitive))
        selected += ".txt";

    QString reason;
    if (!isPathSafeForWrite(selected, reason)) {
        QMessageBox::warning(this, "Save Tasks", reason);
        return;
    }

    if (QFileInfo::exists(selected)) {
        const auto answer = QMessageBox::question(
            this, "Save Tasks",
            QString("Файл\n%1\nуже существует. Перезаписать?").arg(selected),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }

    QString error;
    if (!writeTasksToFile(selected, error)) {
        QMessageBox::critical(this, "Save Tasks",
                              QString("Не удалось сохранить: %1").arg(error));
        return;
    }

    writeLastPath(lastSaveFile, selected);
    QMessageBox::information(this, "Save Tasks",
                             QString("Сохранено в:\n%1").arg(selected));
}

void ToDoListApp::loadTasks() {
    const QString lastLoadFile = pathsDir + "/last_load_path.txt";
    const QString startPath = readLastPath(lastLoadFile, QDir::homePath());

    const QString selected = QFileDialog::getOpenFileName(
        this,
        "Load Tasks",
        startPath,
        "Text files (*.txt);;All files (*)");
    if (selected.isEmpty())
        return;

    const QFileInfo info(selected);
    if (!info.exists() || !info.isFile()) {
        QMessageBox::warning(this, "Load Tasks", "Файл не найден.");
        return;
    }
    if (!info.isReadable()) {
        QMessageBox::warning(this, "Load Tasks", "Нет прав на чтение файла.");
        return;
    }

    QVector<Task> loaded;
    QString error;
    if (!readTasksFromFile(selected, loaded, error)) {
        QMessageBox::critical(this, "Load Tasks",
                              QString("Не удалось прочитать: %1").arg(error));
        return;
    }

    tasks = loaded;
    updateTaskList();
    cacheTasksToFile();

    writeLastPath(lastLoadFile, selected);
    QMessageBox::information(this, "Load Tasks",
                             QString("Загружено %1 задач из:\n%2")
                                 .arg(tasks.size()).arg(selected));
}

// Cache

void ToDoListApp::cacheTasksToFile() {
    QFile cacheFile(cacheFilePath);
    if (!cacheFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    QJsonArray tasksArray;
    for (const Task &task : tasks) {
        QJsonObject taskObject;
        taskObject["description"] = task.getDescription();
        taskObject["comment"] = task.getComment();
        taskObject["completed"] = task.isCompleted();
        taskObject["imagePath"] = task.getImagePath();
        tasksArray.append(taskObject);
    }

    QJsonDocument jsonDocument(tasksArray);
    QTextStream stream(&cacheFile);
    stream << jsonDocument.toJson(QJsonDocument::Indented);
    cacheFile.close();
}

void ToDoListApp::cacheTasksFromCacheFile() {
    QFile cacheFile(cacheFilePath);
    if (!cacheFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    const QByteArray jsonData = cacheFile.readAll();
    cacheFile.close();

    QJsonParseError parseError{};
    const QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDocument.isArray())
        return;

    QVector<Task> loaded;
    for (const QJsonValue &taskValue : jsonDocument.array()) {
        if (!taskValue.isObject())
            continue;
        const QJsonObject taskObject = taskValue.toObject();
        Task task(taskObject["description"].toString(),
                  taskObject["completed"].toBool());
        task.setComment(taskObject["comment"].toString());
        task.setImagePath(taskObject["imagePath"].toString());
        loaded.append(task);
    }

    tasks = loaded;
    updateTaskList();
}

bool ToDoListApp::writeTasksToFile(const QString &path, QString &error) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        error = file.errorString();
        return false;
    }

    auto escape = [](QString s) {
        s.replace('\\', "\\\\");
        s.replace('\t', "\\t");
        s.replace('\n', "\\n");
        return s;
    };

    QTextStream stream(&file);
    for (const Task &task : tasks) {
        stream << escape(task.getDescription()) << '\t'
               << (task.isCompleted() ? '1' : '0') << '\t'
               << escape(task.getImagePath()) << '\t'
               << escape(task.getComment()) << '\n';
    }

    stream.flush();
    if (file.error() != QFileDevice::NoError) {
        error = file.errorString();
        file.close();
        return false;
    }
    file.close();
    return true;
}

bool ToDoListApp::readTasksFromFile(const QString &path,
                                    QVector<Task> &out,
                                    QString &error) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = file.errorString();
        return false;
    }

    auto unescape = [](QString s) {
        s.replace("\\n", "\n");
        s.replace("\\t", "\t");
        s.replace("\\\\", "\\");
        return s;
    };

    QVector<Task> loaded;
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (line.isEmpty())
            continue;

        const QStringList parts = line.split('\t');
        if (parts.size() < 3) {
            error = "Ошибка: выбранный файл не является файлом сохранения "
                    "todo-list-app, либо был поврежден или изменен.";
            file.close();
            return false;
        }

        Task task(unescape(parts[0]), parts[1] == "1");
        task.setImagePath(unescape(parts[2]));
        if (parts.size() >= 4)
            task.setComment(unescape(parts[3]));
        loaded.append(task);
    }

    if (file.error() != QFileDevice::NoError) {
        error = file.errorString();
        file.close();
        return false;
    }

    file.close();
    out = loaded;
    return true;
}