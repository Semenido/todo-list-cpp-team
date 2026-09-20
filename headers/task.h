#ifndef TASK_H
#define TASK_H

#include <QString>
#include <QPixmap>

class Task {
public:
    enum class Priority {
        Low = 0,
        Medium = 1,
        High = 2
    };

    Task(const QString &description, bool completed = false);

    int getId() const { return id; }

    QString getDescription() const;
    void setDescription(const QString &value);

    QString getComment() const;
    void setComment(const QString &value);

    Priority getPriority() const;
    void setPriority(Priority value);

    bool isCompleted() const;
    void toggleComplete();

    void setImagePath(const QString &path);
    QString getImagePath() const;
    QPixmap getImage() const;

    static QString priorityToString(Priority value);
    static Priority priorityFromString(const QString &value);

private:
    static int nextId;
    int id;
    QString description;
    QString comment;
    Priority priority = Priority::Medium;
    bool completed;
    QString imagePath;
};

#endif // TASK_H