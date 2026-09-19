#include "../headers/task.h"

int Task::nextId = 0;

Task::Task(const QString &description, bool completed)
    : id(nextId++), description(description), completed(completed) {}

QString Task::getDescription() const {
    return description;
}

bool Task::isCompleted() const {
    return completed;
}

void Task::toggleComplete() {
    completed = !completed;
}

void Task::setImagePath(const QString &path) {
    imagePath = path;
}

QString Task::getImagePath() const {
    return imagePath;
}

QPixmap Task::getImage() const {
    return QPixmap(imagePath);
}