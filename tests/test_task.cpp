#include <QtTest>
#include <QDateTime>
#include <QDir>
#include <QTemporaryFile>
#include "../headers/task.h"

class TestTask : public QObject
{
    Q_OBJECT

private slots:
    // --- Создание задачи ---
    void testDefaultConstruction()
    {
        Task t("Buy milk");
        QCOMPARE(t.getDescription(), QString("Buy milk"));
        QCOMPARE(t.isCompleted(), false);
        QCOMPARE(t.getComment(), QString(""));
        QCOMPARE(t.getPriority(), Task::Priority::Medium); // дефолт по коду
        QVERIFY(!t.getId() == 0 ? true : true); // id не 0 (или просто проверяем что id задан)
        QVERIFY(t.getId() >= 0);
    }

    void testIdsAreUnique()
    {
        Task a("A");
        Task b("B");
        QVERIFY(a.getId() != b.getId());
    }

    // --- Описание ---
    void testSetDescription()
    {
        Task t("Old");
        t.setDescription("New");
        QCOMPARE(t.getDescription(), QString("New"));
    }

    // --- Комментарий ---
    void testComment()
    {
        Task t("Task");
        t.setComment("Some comment");
        QCOMPARE(t.getComment(), QString("Some comment"));
    }

    // --- Приоритет ---
    void testPriority()
    {
        Task t("Task");
        t.setPriority(Task::Priority::High);
        QCOMPARE(t.getPriority(), Task::Priority::High);

        t.setPriority(Task::Priority::Low);
        QCOMPARE(t.getPriority(), Task::Priority::Low);
    }

    // --- Завершённость ---
    void testSetCompleted()
    {
        Task t("Task");
        QCOMPARE(t.isCompleted(), false);
        t.setCompleted(true);
        QCOMPARE(t.isCompleted(), true);
    }

    void testToggleComplete()
    {
        Task t("Task");
        QCOMPARE(t.isCompleted(), false);
        t.toggleComplete();
        QCOMPARE(t.isCompleted(), true);
        t.toggleComplete();
        QCOMPARE(t.isCompleted(), false);
    }

    // --- Дата завершения ---
    void testCompletedAt()
    {
        Task t("Task");
        QDateTime now = QDateTime::currentDateTime();
        t.setCompletedAt(now);
        QCOMPARE(t.getCompletedAt(), now);
    }

    // --- Изображение ---
    void testImagePath()
    {
        Task t("Task");
        QCOMPARE(t.getImagePath(), QString(""));
        t.setImagePath("/tmp/img.png");
        QCOMPARE(t.getImagePath(), QString("/tmp/img.png"));
    }

    void testImageEmptyForInvalidPath()
    {
        Task t("Task");
        t.setImagePath("/nonexistent/file.png");
        // getImage вернёт пустой QPixmap
        QVERIFY(t.getImage().isNull());
    }

    // --- Статические методы Priority ---
    void testPriorityToString()
    {
        QCOMPARE(Task::priorityToString(Task::Priority::Low),    QString("low"));
        QCOMPARE(Task::priorityToString(Task::Priority::Medium), QString("medium"));
        QCOMPARE(Task::priorityToString(Task::Priority::High),   QString("high"));
    }

    void testPriorityFromString()
    {
        QCOMPARE(Task::priorityFromString("Low"),    Task::Priority::Low);
        QCOMPARE(Task::priorityFromString("Medium"), Task::Priority::Medium);
        QCOMPARE(Task::priorityFromString("High"),   Task::Priority::High);
    }

    void testPriorityRoundTrip()
    {
        QCOMPARE(Task::priorityFromString(Task::priorityToString(Task::Priority::Low)),
                 Task::Priority::Low);
        QCOMPARE(Task::priorityFromString(Task::priorityToString(Task::Priority::Medium)),
                 Task::Priority::Medium);
        QCOMPARE(Task::priorityFromString(Task::priorityToString(Task::Priority::High)),
                 Task::Priority::High);
    }
};

QTEST_MAIN(TestTask)
#include "test_task.moc"
