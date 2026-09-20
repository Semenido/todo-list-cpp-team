#ifndef TASK_H
#define TASK_H

#include <QString>
#include <QPixmap>

class Task {
public:
    Task(const QString &description, bool completed = false);

    int getId() const { return id; }

    QString getDescription() const;
    void setDescription(const QString &value);

    QString getComment() const;
    void setComment(const QString &value);

    bool isCompleted() const;
    void toggleComplete();

    void setImagePath(const QString &path);
    QString getImagePath() const;
    QPixmap getImage() const;

private:
    static int nextId;
    int id;
    QString description;
    QString comment;
    bool completed;
    QString imagePath;
};

#endif // TASK_H