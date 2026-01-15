#include "taskwarrior.hpp"

#include <QDebug>
#include <QList>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

#include "configmanager.hpp"
#include "date_time_parser.hpp"
#include "task.hpp"
#include "taskwarriorexecutor.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>

Taskwarrior::Taskwarrior()
    : m_actions_counter(0ull)
    , m_executor(nullptr)
    , m_filter({})
{
}

Taskwarrior::~Taskwarrior() = default;

bool Taskwarrior::init()
{
    const auto &binary = ConfigManager::config().getTaskBin();
    try {
        m_executor = std::make_unique<TaskWarriorExecutor>(binary);
        return true;
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return false;
}

bool Taskwarrior::addTask(DetailedTaskInfo &task)
{
    return execCommandAndAccountUndo(
        [this, &task]() { return task.execAddNewTask(*m_executor); });
}

bool Taskwarrior::startTasks(const QList<DetailedTaskInfo> &tasks)
{
    return execCommandAndAccountUndo([this, &tasks]() {
        return BatchTasksManager(tasks).execStartTask(*m_executor);
    });
}

bool Taskwarrior::stopTasks(const QList<DetailedTaskInfo> &tasks)
{
    return execCommandAndAccountUndo([this, &tasks]() {
        return BatchTasksManager(tasks).execStopTask(*m_executor);
    });
}

bool Taskwarrior::editTask(DetailedTaskInfo &task)
{
    return execCommandAndAccountUndo(
        [this, &task]() { return task.execModifyExisting(*m_executor); });
}

bool Taskwarrior::setPriority(const QString &id, DetailedTaskInfo::Priority p)
{
    DetailedTaskInfo task(id);
    task.priority = p;
    return editTask(task);
}

std::optional<DetailedTaskInfo> Taskwarrior::getTask(const QString &id)
{
    DetailedTaskInfo task(id);
    if (task.execReadExisting(*m_executor)) {
        return task;
    }
    return std::nullopt;
}

bool Taskwarrior::deleteTask(const QString &id)
{
    return execCommandAndAccountUndo([this, &id]() {
        return BatchTasksManager({ id }).execDeleteTask(*m_executor);
    });
}

bool Taskwarrior::deleteTask(const QStringList &ids)
{
    return execCommandAndAccountUndo([this, &ids]() {
        return BatchTasksManager(ids).execDeleteTask(*m_executor);
    });
}

bool Taskwarrior::setTaskDone(const QString &id)
{
    return execCommandAndAccountUndo([this, &id]() {
        return BatchTasksManager({ id }).execDoneTask(*m_executor);
    });
}

bool Taskwarrior::setTaskDone(const QStringList &ids)
{
    return execCommandAndAccountUndo([this, &ids]() {
        return BatchTasksManager(ids).execDoneTask(*m_executor);
    });
}

bool Taskwarrior::waitTask(const QString &id, const QDateTime &datetime)
{
    return execCommandAndAccountUndo([&]() {
        return BatchTasksManager({ id }).execWaitTask(datetime, *m_executor);
    });
}

bool Taskwarrior::waitTask(const QStringList &ids, const QDateTime &datetime)
{
    return execCommandAndAccountUndo([&]() {
        return BatchTasksManager(ids).execWaitTask(datetime, *m_executor);
    });
}

std::optional<QList<DetailedTaskInfo>> Taskwarrior::getUrgencySortedTasks()
{
    FilteredTasksListReader retriever(m_filter);
    if (m_executor && retriever.readUrgencySortedTaskList(*m_executor)) {
        return std::move(retriever.tasks);
    }
    return std::nullopt;
}

std::optional<QList<RecurringTaskTemplate>>
Taskwarrior::getRecurringTasks() const
{
    if (!m_executor) {
        return std::nullopt;
    }
    return RecurringTaskTemplate::readAll(*m_executor);
}

bool Taskwarrior::undoTask()
{
    if (m_actions_counter == 0) {
        return true;
    }
    if (m_executor && m_executor->execTaskProgramWithDefaults({ "undo" })) {
        --m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::applyFilter(QStringList user_keywords)
{
    if (!m_executor) {
        return false;
    }
    const bool is_empty = user_keywords.empty();
    m_filter = AllAtOnceKeywordsFinder(std::move(user_keywords));
    if (is_empty) {
        return true;
    }
    return m_filter.readIds(*m_executor);
}

int Taskwarrior::directCmd(const QString &cmd)
{
    if (!m_executor) {
        return -1;
    }
    return m_executor
        ->execTaskProgramWithDefaults(
            DateTimeParser::splitSpaceSeparatedString(cmd))
        .getError()
        .code;
}
