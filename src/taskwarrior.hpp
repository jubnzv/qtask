#ifndef TASKWARRIOR_HPP
#define TASKWARRIOR_HPP

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "task.hpp"
#include "taskwarriorexecutor.hpp"

#include <cstddef>
#include <memory>
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
        if (m_executor) {
            return m_executor->getTaskVersion();
        }
        return {};
    }

    bool addTask(DetailedTaskInfo &task);
    bool startTask(const QString &id);
    bool stopTask(const QString &id);
    bool editTask(DetailedTaskInfo &task);
    bool setPriority(const QString &id, DetailedTaskInfo::Priority);
    [[nodiscard]] std::optional<DetailedTaskInfo> getTask(const QString &id);
    [[nodiscard]] std::optional<QList<DetailedTaskInfo>> getTasks();
    [[nodiscard]] std::optional<QList<RecurringTaskTemplate>>
    getRecurringTasks() const;
    bool deleteTask(const QString &id);
    bool deleteTask(const QStringList &ids);
    bool setTaskDone(const QString &id);
    bool setTaskDone(const QStringList &ids);
    bool waitTask(const QString &id, const QDateTime &datetime);
    bool waitTask(const QStringList &ids, const QDateTime &datetime);
    bool undoTask();

    bool applyFilter(QStringList user_keywords);

    int directCmd(const QString &cmd);

  private:
    bool getActiveIds(QStringList &result);

    template <typename taCallable>
    bool execCommandAndAccountUndo(const taCallable &callable)
    {
        if (m_executor && callable()) {
            ++m_actions_counter;
            return true;
        }
        return false;
    }

  private:
    /// @brief Counter of changes in the taskwarrior database that can be
    /// undone.
    size_t m_actions_counter;
    std::unique_ptr<TaskWarriorExecutor> m_executor;
    AllAtOnceKeywordsFinder m_filter;
};

#endif // TASKWARRIOR_HPP
