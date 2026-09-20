#include "../headers/task.h"

int Task::nextId = 0;

Task::Task(const QString &description, bool completed)
    : id(nextId++), description(description), completed(completed) {}

QString Task::getDescription() const {
    return description;
}

void Task::setDescription(const QString &value) {
    description = value;
}

QString Task::getComment() const {
    return comment;
}

void Task::setComment(const QString &value) {
    comment = value;
}

Task::Priority Task::getPriority() const {
    return priority;
}

void Task::setPriority(Priority value) {
    priority = value;
}

bool Task::isCompleted() const {
    return completed;
}

void Task::setCompleted(bool value) {
    completed = value;
}

void Task::toggleComplete() {
    completed = !completed;
}

QDateTime Task::getCompletedAt() const {
    return completedAt;
}

void Task::setCompletedAt(const QDateTime &value) {
    completedAt = value;
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

QString Task::priorityToString(Priority value) {
    switch (value) {
    case Priority::Low:    return "low";
    case Priority::High:   return "high";
    case Priority::Medium:
    default:               return "medium";
    }
}

Task::Priority Task::priorityFromString(const QString &value) {
    const QString v = value.trimmed().toLower();
    if (v == "low")    return Priority::Low;
    if (v == "high")   return Priority::High;
    return Priority::Medium;
}