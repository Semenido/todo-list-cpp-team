#ifndef EDITTASKDIALOG_H
#define EDITTASKDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
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

private slots:
    void onAccept();

private:
    QLabel *descriptionLabel;
    QLineEdit *descriptionEdit;
    QLabel *commentLabel;
    QPlainTextEdit *commentEdit;
    QLabel *priorityLabel;
    QComboBox *priorityCombo;
};

#endif // EDITTASKDIALOG_H