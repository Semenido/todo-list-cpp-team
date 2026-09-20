#include "../headers/edittaskdialog.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

EditTaskDialog::EditTaskDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Edit Task");
    setModal(true);
    resize(480, 360);

    auto *layout = new QVBoxLayout(this);

    descriptionLabel = new QLabel("Task name", this);
    descriptionEdit = new QLineEdit(this);
    descriptionEdit->setPlaceholderText("Task name (required)");
    layout->addWidget(descriptionLabel);
    layout->addWidget(descriptionEdit);

    commentLabel = new QLabel("Comment", this);
    commentEdit = new QPlainTextEdit(this);
    commentEdit->setPlaceholderText("Optional comment...");
    layout->addWidget(commentLabel);
    layout->addWidget(commentEdit, 1);

    priorityLabel = new QLabel("Priority", this);
    priorityCombo = new QComboBox(this);
    priorityCombo->addItem("Low",    QVariant::fromValue(static_cast<int>(Task::Priority::Low)));
    priorityCombo->addItem("Medium", QVariant::fromValue(static_cast<int>(Task::Priority::Medium)));
    priorityCombo->addItem("High",   QVariant::fromValue(static_cast<int>(Task::Priority::High)));
    priorityCombo->setCurrentIndex(1);   // Medium по умолчанию
    layout->addWidget(priorityLabel);
    layout->addWidget(priorityCombo);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &EditTaskDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &EditTaskDialog::reject);
}

void EditTaskDialog::setDescription(const QString &value) {
    descriptionEdit->setText(value);
}

QString EditTaskDialog::description() const {
    return descriptionEdit->text().trimmed();
}

void EditTaskDialog::setComment(const QString &value) {
    commentEdit->setPlainText(value);
}

QString EditTaskDialog::comment() const {
    return commentEdit->toPlainText();
}

void EditTaskDialog::setPriority(Task::Priority value) {
    const int idx = priorityCombo->findData(static_cast<int>(value));
    if (idx >= 0)
        priorityCombo->setCurrentIndex(idx);
}

Task::Priority EditTaskDialog::priority() const {
    return static_cast<Task::Priority>(
        priorityCombo->currentData().toInt());
}

void EditTaskDialog::onAccept() {
    if (description().isEmpty()) {
        QMessageBox::warning(this, "Edit Task",
                             "Task name must not be empty.");
        descriptionEdit->setFocus();
        return;
    }
    accept();
}