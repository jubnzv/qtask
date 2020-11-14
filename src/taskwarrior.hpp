#ifndef TASKWARRIOR_HPP
#define TASKWARRIOR_HPP

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>

#include "task.hpp"

class Taskwarrior {
  public:
    Taskwarrior();
    ~Taskwarrior();

    size_t getActionsCounter() const { return m_actions_counter; }

    bool addTask(const Task &task);
    bool startTask(const QString &id);
    bool stopTask(const QString &id);
    bool editTask(const Task &task);
    bool setPriority(const QString &id, Task::Priority);
    bool getTask(const QString &id, Task &out_task);
    bool getTasks(QList<Task> &task);
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
    bool execCmd(const QStringList &args, bool filter_enabled = false);
    bool execCmd(const QStringList &args, QByteArray &out,
                 bool filter_enabled = false);

    bool getActiveIds(QStringList &result);

    QString formatDateTime(const QDateTime &) const;

  private:
    static const QStringList s_args;

    QStringList m_filter;
    /// Counter of changes in the taskwarrior database that can be undone.
    size_t m_actions_counter;
};

#endif // TASKWARRIOR_HPP
