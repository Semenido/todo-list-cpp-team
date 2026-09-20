#include "../headers/todolistapp.h"
#include "../headers/edittaskdialog.h"
#include <QVBoxLayout>
#include <QMenu>
#include <QAction>
#include <QPoint>
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
#include <QFont>
#include <algorithm>

static const char *kSettingsFileName = "settings.json";
static const char *kLastSaveFileKey  = "last_save_path";
static const char *kLastLoadFileKey  = "last_load_path";
static const char *kSortModeKey      = "sort_mode";

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
    QDir().mkpath(baseDir);

    cacheFilePath    = baseDir + "/cached_tasks.json";
    settingsFilePath = baseDir + "/" + kSettingsFileName;

    loadSettings();
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

QString ToDoListApp::priorityBadgeColor(Task::Priority p) {
    switch (p) {
    case Task::Priority::Low:    return "#9e9e9e";
    case Task::Priority::High:   return "#e53935";
    case Task::Priority::Medium:
    default:                     return "#fb8c00";
    }
}

QString ToDoListApp::priorityLabel(Task::Priority p) {
    switch (p) {
    case Task::Priority::Low:    return "Low";
    case Task::Priority::High:   return "High";
    case Task::Priority::Medium:
    default:                     return "Medium";
    }
}

void ToDoListApp::updateTaskList() {
    updatingList = true;
    taskList->clear();

    QVector<Task> ordered = tasks;
    switch (sortMode) {
    case SortMode::ByPriority:
        std::stable_sort(ordered.begin(), ordered.end(),
                         [](const Task &a, const Task &b) {
                             return static_cast<int>(a.getPriority())
                                  > static_cast<int>(b.getPriority());
                         });
        break;
    case SortMode::ByName:
        std::stable_sort(ordered.begin(), ordered.end(),
                         [](const Task &a, const Task &b) {
                             return a.getDescription().localeAwareCompare(
                                        b.getDescription()) < 0;
                         });
        break;
    case SortMode::None:
    default:
        break;
    }

    for (const Task &task : ordered) {
        auto *item = new QListWidgetItem(task.getDescription());
        item->setData(Qt::UserRole, task.getId());

        if (!task.getComment().isEmpty())
            item->setToolTip(task.getComment());

        if (task.getPriority() == Task::Priority::High) {
            QFont f = item->font();
            f.setBold(true);
            item->setFont(f);
        }

        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(task.isCompleted() ? Qt::Checked : Qt::Unchecked);
        taskList->addItem(item);

        const QString badgeText =
            QString("<span style='color:%1; font-weight:bold;'>●</span> %2")
                .arg(priorityBadgeColor(task.getPriority()),
                     priorityLabel(task.getPriority()));

        auto *badge = new QLabel(badgeText);
        badge->setTextFormat(Qt::RichText);
        badge->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
        badge->setContentsMargins(0, 0, 6, 0);
        taskList->setItemWidget(item, badge);
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

void ToDoListApp::onContextMenuRequested(const QPoint &pos) {
    QListWidgetItem *item = taskList->itemAt(pos);

    QMenu menu(this);

    if (item) {
        taskList->setCurrentItem(item);
        menu.addAction("Редактировать");
        menu.addSeparator();
    }

    QMenu *sortMenu = menu.addMenu("Сортировка");

    QAction *byPriorityAction = sortMenu->addAction("Сортировать по приоритету");
    byPriorityAction->setCheckable(true);
    byPriorityAction->setChecked(sortMode == SortMode::ByPriority);

    QAction *byNameAction = sortMenu->addAction("Сортировать по имени");
    byNameAction->setCheckable(true);
    byNameAction->setChecked(sortMode == SortMode::ByName);

    QAction *chosen = menu.exec(taskList->viewport()->mapToGlobal(pos));
    if (!chosen)
        return;

    if (chosen == byPriorityAction) {
        sortMode = (sortMode == SortMode::ByPriority)
                       ? SortMode::None
                       : SortMode::ByPriority;
    } else if (chosen == byNameAction) {
        sortMode = (sortMode == SortMode::ByName)
                       ? SortMode::None
                       : SortMode::ByName;
    } else {
        return;
    }

    saveSettings();
    updateTaskList();
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
    dialog.setPriority(t->getPriority());

    if (dialog.exec() != QDialog::Accepted)
        return;

    const QString newDescription = dialog.description();
    const QString newComment = dialog.comment();
    const Task::Priority newPriority = dialog.priority();

    if (newDescription == t->getDescription()
        && newComment == t->getComment()
        && newPriority == t->getPriority())
        return;

    t->setDescription(newDescription);
    t->setComment(newComment);
    t->setPriority(newPriority);

    updateTaskList();
    cacheTasksToFile();
}

// ---------------- settings ----------------

void ToDoListApp::loadSettings() {
    QFile f(settingsFilePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return;

    const QJsonObject obj = doc.object();

    sortMode = sortModeFromString(obj.value(kSortModeKey).toString());
}

void ToDoListApp::saveSettings() const {
    QFile f(settingsFilePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    QJsonObject obj;
    obj[kSortModeKey] = sortModeToString(sortMode);

    const QJsonDocument doc(obj);
    QTextStream stream(&f);
    stream << doc.toJson(QJsonDocument::Indented);
    f.close();
}

QString ToDoListApp::sortModeToString(SortMode mode) {
    switch (mode) {
    case SortMode::ByPriority: return "priority";
    case SortMode::ByName:     return "name";
    case SortMode::None:
    default:                   return "none";
    }
}

ToDoListApp::SortMode ToDoListApp::sortModeFromString(const QString &value) {
    const QString v = value.trimmed().toLower();
    if (v == "priority") return SortMode::ByPriority;
    if (v == "name")     return SortMode::ByName;
    return SortMode::None;
}

// ---------------- last path (в settings.json) ----------------

QString ToDoListApp::readLastPath(const QString &filePath,
                                  const QString &fallback) const {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return fallback;

    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return fallback;

    const QString key = (filePath == "save") ? kLastSaveFileKey : kLastLoadFileKey;
    const QString line = doc.object().value(key).toString().trimmed();
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
    QFile f(settingsFilePath);
    QJsonObject obj;

    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QByteArray data = f.readAll();
        f.close();
        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject())
            obj = doc.object();
    }

    const QString key = (filePath == "save") ? kLastSaveFileKey : kLastLoadFileKey;
    obj[key] = value;
    obj[kSortModeKey] = sortModeToString(sortMode);

    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    QTextStream stream(&f);
    stream << QJsonDocument(obj).toJson(QJsonDocument::Indented);
    f.close();
}

// ---------------- Save / Load ----------------

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
               << escape(task.getComment()) << '\t'
               << Task::priorityToString(task.getPriority()) << '\n';
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
        if (parts.size() >= 5)
            task.setPriority(Task::priorityFromString(unescape(parts[4])));
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
    const QString startPath = readLastPath("save", QDir::homePath());

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

    writeLastPath("save", selected);
    QMessageBox::information(this, "Save Tasks",
                             QString("Сохранено в:\n%1").arg(selected));
}

void ToDoListApp::loadTasks() {
    const QString startPath = readLastPath("load", QDir::homePath());

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

    writeLastPath("load", selected);
    QMessageBox::information(this, "Load Tasks",
                             QString("Загружено %1 задач из:\n%2")
                                 .arg(tasks.size()).arg(selected));
}

// ---------------- Cache ----------------

void ToDoListApp::cacheTasksToFile() {
    QFile cacheFile(cacheFilePath);
    if (!cacheFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return;

    QJsonArray tasksArray;
    for (const Task &task : tasks) {
        QJsonObject taskObject;
        taskObject["description"] = task.getDescription();
        taskObject["comment"] = task.getComment();
        taskObject["priority"] = Task::priorityToString(task.getPriority());
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
        task.setPriority(Task::priorityFromString(
            taskObject["priority"].toString()));
        task.setImagePath(taskObject["imagePath"].toString());
        loaded.append(task);
    }

    tasks = loaded;
    updateTaskList();
}