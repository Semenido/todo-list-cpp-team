#ifndef EDITTASKDIALOG_H
#define EDITTASKDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QLabel>
#include "task.h"

class EditTaskDialog : public QDialog {
    Q_OBJECT
public:
    explicit EditTaskDialog(QWidget *parent = nullptr);

    void setDescription(const QString &value);
    QString description() const;

    void setComment(const QString &value);
    QString comment() const;

    void setPriority(Task::Priority value);
    Task::Priority priority() const;

    void setTaskCompleted(bool value);
    bool taskCompleted() const;

    void setCompletedAt(const QDateTime &value);
    QDateTime completedAt() const;

private slots:
    void onAccept();
    void onCompletedToggled(bool checked);

private:
    QLabel *descriptionLabel;
    QLineEdit *descriptionEdit;

    QLabel *commentLabel;
    QPlainTextEdit *commentEdit;

    QLabel *priorityLabel;
    QComboBox *priorityCombo;

    QLabel *completedLabel;
    QCheckBox *completedCheck;

    QLabel *completedAtLabel;
    QDateTimeEdit *completedAtEdit;
};

#endif // EDITTASKDIALOG_H