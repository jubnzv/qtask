#ifndef TASKWARRIOR_HPP
#define TASKWARRIOR_HPP

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "task.hpp"

#include <cstddef>
#include <optional>

class Taskwarrior {
  public:
    Taskwarrior();
    ~Taskwarrior();
    Taskwarrior(const Taskwarrior &) = delete;
    Taskwarrior &operator=(const Taskwarrior &) = delete;
    Taskwarrior(Taskwarrior &&) = delete;
    Taskwarrior &operator=(Taskwarrior &&) = delete;

    /// Detect the version of task and check that it works. This function
    /// will also runs garbage collection. This will un-waiting tasks and add
    /// new recurring tasks.
    bool init();

    [[nodiscard]]
    size_t getActionsCounter() const
    {
        return m_actions_counter;
    }

    [[nodiscard]]
    QVariant getTaskVersion() const
    {
        return m_task_version;
    }

    bool addTask(const Task &task);
    bool startTask(const QString &id);
    bool stopTask(const QString &id);
    bool editTask(const Task &task);
    bool setPriority(const QString &id, Task::Priority);
    [[nodiscard]] std::optional<Task> getTask(const QString &id) const;
    [[nodiscard]] std::optional<QList<Task>> getTasks() const;
    [[nodiscard]] std::optional<QList<RecurringTask>> getRecurringTasks() const;
    bool deleteTask(const QString &id);
    bool deleteTask(const QStringList &ids);
    bool setTaskDone(const QString &id);
    bool setTaskDone(const QStringList &ids);
    bool waitTask(const QString &id, const QDateTime &datetime);
    bool waitTask(const QStringList &ids, const QDateTime &datetime);
    bool undoTask();

    bool applyFilter(const QStringList &);

    int directCmd(const QString &cmd);

  private:
    bool getActiveIds(QStringList &result);

  private:
    QStringList m_filter;

    /// Counter of changes in the taskwarrior database that can be undone.
    size_t m_actions_counter;

    QVariant m_task_version;
};

#endif // TASKWARRIOR_HPP
