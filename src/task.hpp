#ifndef TASK_HPP
#define TASK_HPP

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <qtypes.h>
#include <qvariant.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <tuple>

#include "recurrence_instance_data.hpp"
#include "task_date_time.hpp"
#include "taskproperty.hpp"
#include "taskwarriorexecutor.hpp"

// This should contain ALL TaskProperty<> fields in DetailedTaskInfo.
// Description must be LAST, as it goes multiline sometimes.
#define TASK_PROPERTIES_LIST                                              \
    priority, project, tags, sched, due, wait, active, recurrency_period, \
        description

/// @note Classes here are responsible to produce proper commands to the
/// taskwarrior and parse it's results intact with own fields. Actual execution
/// of the commands is done elsewhere via TaskWarriorExecutor.

///@brief Represents single task.
class DetailedTaskInfo {
  public:
    // TODO: revise if it should stay public.
    enum class Priority : std::uint8_t { Unset, L, M, H }; // TODO: properties
    static Priority priorityFromString(const QString &p);

    // task_id and task_uuid are special, it is always used explicit, so we do
    // not need to track modification.
    // task_id is numeric ID currently assigned to task, it can be changed by
    // taskwarriror on ins/del operations.
    QString task_id;
    // task_uuid is unique long id, it must remain the same always (I guess).
    QString task_uuid;

    // Note, update TASK_PROPERTIES_LIST macros if you add/remove some
    // here.
    TaskProperty<QString> description;
    TaskProperty<QString> project;
    TaskProperty<QStringList> tags; // current tags list
    TaskProperty<TaskDateTime<ETaskDateTimeRole::Sched>> sched;
    TaskProperty<TaskDateTime<ETaskDateTimeRole::Due>> due;
    TaskProperty<TaskDateTime<ETaskDateTimeRole::Wait>> wait;
    TaskProperty<Priority> priority;
    TaskProperty<bool> active;
    TaskProperty<RecurrentInstancePeriod> recurrency_period;

    /// @brief Copies different fields from @p other object. If field was equal,
    /// keeps "modified" state as it was before.
    void updateFrom(const DetailedTaskInfo &other);

    /// @brief tries to add this object as new task to taskwarrior.
    /// @returns true if task was added
    /// @note Does not require task_id field initially set and field is not
    /// updated.
    bool execAddNewTask(const TaskWarriorExecutor &executor);

    /// @brief uses this object to update existing task in taskwarrior.
    /// @note All fields of this are written.
    /// @returns true if this object updated data in taskwarrior.
    bool execModifyExisting(const TaskWarriorExecutor &executor);

    /// @brief Tries to read all data from task_warrior and fill this object.
    /// @note task_id field must be set prior the call.
    /// @returns true if object was properly read.
    bool execReadExisting(const TaskWarriorExecutor &executor);

    /// @returns true if this object has all possible data read from `task`.
    [[nodiscard]]
    bool isFullRead() const;

    void markFullRead() { dataState = ReadAs::FullRead; }

    [[nodiscard]]
    static constexpr std::size_t propertiesCount()
    {
        return std::tuple_size_v<decltype(std::tie(TASK_PROPERTIES_LIST))>;
    }

  public:
    /// @brief Constructs object with defaults, except given @p task_id set.
    explicit DetailedTaskInfo(QString task_id);
    /// @brief Constructs object with defaults, except given @p task_id set.
    explicit DetailedTaskInfo(int task_id);

    DetailedTaskInfo();

  private:
    enum class ReadAs : std::uint8_t {
        ParticularRead,
        FullRead,
    };

    ReadAs dataState{ ReadAs::ParticularRead };

    [[nodiscard]]
    QStringList getAddModifyCmdArgsFieldsRepresentation() const;

    [[nodiscard]]
    auto asTuple()
    {
        return std::tie(TASK_PROPERTIES_LIST);
    }
    [[nodiscard]] auto asTuple() const
    {
        return std::tie(TASK_PROPERTIES_LIST);
    }
};

Q_DECLARE_METATYPE(DetailedTaskInfo)

template <typename taTasksContainer, typename taTaskIdGetter>
QStringList tasksListToIds(const taTasksContainer &tasks,
                           const taTaskIdGetter &idGetter)
{
    QStringList ids;
    ids.reserve(static_cast<qsizetype>(
        std::distance(std::begin(tasks), std::end(tasks))));
    std::transform(std::begin(tasks), std::end(tasks), std::back_inserter(ids),
                   idGetter);
    return ids;
}

/// @brief This object allows Start/Stop/Done/Delete task(s) (1 or more at
/// once).
class BatchTasksManager {
  public:
    explicit BatchTasksManager(const QList<DetailedTaskInfo> &tasks);
    explicit BatchTasksManager(QStringList tasks);

    /// @returns true if call succeed.
    [[nodiscard]]
    bool execDeleteTask(const TaskWarriorExecutor &executor) const;

    [[nodiscard]]
    bool execStartTask(const TaskWarriorExecutor &executor) const;

    [[nodiscard]]
    bool execStopTask(const TaskWarriorExecutor &executor) const;

    [[nodiscard]]
    bool execDoneTask(const TaskWarriorExecutor &executor) const;

    [[nodiscard]]
    bool execWaitTask(const QDateTime &datetime,
                      const TaskWarriorExecutor &executor) const;

  private:
    QStringList m_tasks_ids;

    [[nodiscard]]
    bool execVerb(const QString &verb, const TaskWarriorExecutor &executor,
                  const QString &after_ids = {}) const;
};

/// @brief Issues `task stat` and uses some fields of the response to understand
/// if we must reload data from taskwarrior.
class TaskWarriorDbState {
  public:
    using Optional = std::optional<TaskWarriorDbState>;
    struct DataFields {
        ulong fieldTotal{ 0u };
        ulong fieldUndo{ 0u };
        ulong fieldBacklog{ 0u };
        bool operator!=(const DataFields &other) const
        {
            return !(*this == other);
        }

        bool operator==(const DataFields &other) const
        {
            static const auto Tie = [](const DataFields &what) {
                return std::tie(what.fieldBacklog, what.fieldTotal,
                                what.fieldUndo);
            };
            return Tie(*this) == Tie(other);
        }
    };

    TaskWarriorDbState() = default;
    [[nodiscard]]
    bool isDifferent(const TaskWarriorDbState &other) const
    {
        return this->fields != other.fields;
    }

    [[nodiscard]]
    ulong getUndoCount() const
    {
        return fields.fieldUndo;
    }

    /// @returns std::nullopt if it was error reading stats or current state of
    /// TaskWarrior's data base as TaskWarriorDbState.
    /// @note We select only fields we think can help us to detect modifications
    /// when we should update our GUI.
    [[nodiscard]]
    static Optional readCurrent(const TaskWarriorExecutor &executor);

    /// @returns state which cannot be valid.
    static TaskWarriorDbState invalidState();

  private:
    explicit TaskWarriorDbState(TaskWarriorDbState::DataFields fields)
        : fields(fields)
    {
    }
    DataFields fields;
};

#endif // TASK_HPP
