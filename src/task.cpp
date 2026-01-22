#include "task.hpp"

#include "date_time_parser.hpp"
#include "qtutil.hpp"
#include "recurrence_instance_data.hpp"
#include "split_string.hpp"
#include "tabular_stencil_base.hpp"
#include "task_date_time.hpp"
#include "task_ids_providers.hpp"
#include "taskwarriorexecutor.hpp"

#include <QDateTime>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QStringLiteral>
#include <QVariant>
#include <QtAssert>
#include <qnamespace.h>
#include <qtypes.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace
{

QString makeParam(const QString &field, const QString &value)
{
    // NOTE:
    // QProcess does NOT use shell. Arguments must be passed verbatim.
    // Do NOT add quotes or escaping here.
    Q_ASSERT(!field.isEmpty());
    Q_ASSERT(!field.contains(' '));
    Q_ASSERT(!field.contains(':'));
    return field + ":" + value;
}

QChar priorityToChar(const DetailedTaskInfo::Priority &p)
{
    switch (p) {
    case DetailedTaskInfo::Priority::L:
        return 'L';
    case DetailedTaskInfo::Priority::M:
        return 'M';
    case DetailedTaskInfo::Priority::H:
        return 'H';
    case DetailedTaskInfo::Priority::Unset:
        break;
    }
    throw std::runtime_error("Unset priority cannot be converted.");
}

QString formatTags(const QStringList &tags) // NOLINT
{
    return makeParam("tag", tags.join(" "));
}

QString formatDescription(const QString &descr)
{
    QString clean = descr.trimmed();
    clean.replace(R"(")", "'");
    return makeParam("description", QString(R"("%1")").arg(clean));
}

QString formatPriority(const DetailedTaskInfo::Priority pri)
{
    if (pri != DetailedTaskInfo::Priority::Unset) {
        return makeParam("pri", QString(priorityToChar(pri)));
    } else {
        return makeParam("pri", "");
    }
}

QString formatProject(const QString &proj)
{
    return makeParam("project", proj);
}

template <ETaskDateTimeRole taRole>
QString formatDateTime(const TaskDateTime<taRole> &dt)
{
    return makeParam(TaskDateTime<taRole>::role_name_cmd(),
                     dt.has_value() ? dt->toString(Qt::ISODate) : QString());
}

// Converts many properties into 1 command line.
template <typename taLeft, typename... taMany>
void AppendPropertiesToCmdList(QStringList &output, const taLeft &leftProperty,
                               taMany &&...manyProperties)
{
    if (leftProperty.value.isModified()) {
        output << leftProperty.getStringsForCmd();
    }
    if constexpr (sizeof...(manyProperties) > 0) {
        AppendPropertiesToCmdList(output,
                                  std::forward<taMany>(manyProperties)...);
    }
}

// Cleanses "modified" flag for all given properties.
template <typename taLeft, typename... taMany>
bool SetPropertiesNotChanged(const bool wasTaskCallOk, taLeft &leftProperty,
                             taMany &&...manyProperties)
{
    if (wasTaskCallOk) {
        leftProperty.value.setNotModified();
        if constexpr (sizeof...(manyProperties) > 0) {
            SetPropertiesNotChanged(wasTaskCallOk,
                                    std::forward<taMany>(manyProperties)...);
        }
    }
    return wasTaskCallOk;
}

// Expected values in reading TaskWarrior responses.
constexpr qsizetype kRowIndexOfDividers =
    TabularStencilBase<void>::kRowIndexOfDividers;
constexpr qsizetype kHeadersSize = TabularStencilBase<void>::kHeadersSize;
constexpr qsizetype kFooterSize = TabularStencilBase<void>::kFooterSize;

// returns true if task outputed something except header and footer.
bool isTaskSentData(const QStringList &task_output)
{
    return TabularStencilBase<void>::isTaskSentData(task_output);
}

template <qsizetype taExpectedColumnsAmount>
using OptionalPositions =
    std::optional<std::array<qsizetype, taExpectedColumnsAmount>>;

/// @brief Finds in-string offsets of the "|" which are columns separator in the
/// header.
template <qsizetype taExpectedColumnsAmount>
OptionalPositions<taExpectedColumnsAmount>
findColumnPositions(const QString &line)
{
    std::array<qsizetype, taExpectedColumnsAmount> positions{ 0 };
    qsizetype current_size = 0;

    for (qsizetype i = 0, sz = line.size(); i < sz; ++i) {
        if (line.at(i) == QChar('|')) {
            if (current_size >= taExpectedColumnsAmount) {
                return std::nullopt; // Too many of columns.
            }
            positions.at(current_size++) = i;
        }
    }
    if (current_size != taExpectedColumnsAmount) {
        return std::nullopt; // Not enough of columns.
    }
    return std::make_optional(std::move(positions));
}

/// @brief This class sets field of DetailedTaskInfo based on provided
/// SplitString out of "information" response. Call setField() for each
/// line/field.
class InformationResponseSetters {
  public:
    explicit InformationResponseSetters(DetailedTaskInfo &task)
        : task(task)
    {
    }

    /// @brief Sets field in DetailedTaskInfo based on provided split_string if
    /// it could be parsed properly, otherwise does nothing.
    void setField(const SplitString &split_string)
    {
        using handler_t =
            void (InformationResponseSetters::*)(const SplitString &);
        const std::unordered_map<QString, handler_t> kTagToHandler = {
            { "ID", &InformationResponseSetters::handleID },
            { "UUID", &InformationResponseSetters::handleUUID },
            { "Project", &InformationResponseSetters::handleProject },
            { "Priority", &InformationResponseSetters::handlePriority },
            { "Tags", &InformationResponseSetters::handleTags },
            { "Start", &InformationResponseSetters::handleStart },
            { "Waiting", &InformationResponseSetters::handleWaiting },
            { "Scheduled", &InformationResponseSetters::handleScheduled },
            { "Due", &InformationResponseSetters::handleDue },
            { "Recurrence", &InformationResponseSetters::handleRecurrence },
        };

        const auto it = kTagToHandler.find(split_string.key);
        if (it != kTagToHandler.end()) {
            const handler_t handler = it->second;
            (this->*handler)(split_string);
        }
    }

  private:
    DetailedTaskInfo &task;

    // Handlers for each key.
    void handleID(const SplitString &line) { task.task_id = line.value; }
    void handleUUID(const SplitString &line) { task.task_uuid = line.value; }
    void handleProject(const SplitString &line) { task.project = line.value; }
    void handleStart(const SplitString &) { task.active = true; }
    void handlePriority(const SplitString &line)
    {
        task.priority = DetailedTaskInfo::priorityFromString(line.value);
    }
    void handleTags(const SplitString &line)
    {
        task.tags = DateTimeParser::splitSpaceSeparatedString(line.value);
    }
    void handleWaiting(const SplitString &line)
    {
        // Waiting until 2025-11-04 00:00:00
        // so "until" word becomes 0th token of SplitString::value
        static const DateTimeParser parser{ 3, 1, 2 };
        task.wait =
            parser.parseDateTimeString<ETaskDateTimeRole::Wait>(line.value);
    }
    void handleScheduled(const SplitString &line)
    {
        static const DateTimeParser parser{ 2, 0, 1 };
        task.sched =
            parser.parseDateTimeString<ETaskDateTimeRole::Sched>(line.value);
    }
    void handleDue(const SplitString &line)
    {
        static const DateTimeParser parser{ 2, 0, 1 };
        task.due =
            parser.parseDateTimeString<ETaskDateTimeRole::Due>(line.value);
    }
    void handleRecurrence(const SplitString &line)
    {
        // class RecurrentInstancePeriod does not want empty lines.
        if (!line.value.isEmpty() && !line.value.contains("type")) {
            task.recurrency_period.value.modify(
                [&line](RecurrentInstancePeriod &period) {
                    period.setRecurrent(line.value);
                });
        }
    }
};

class StatResponseSetters {
  public:
    explicit StatResponseSetters(TaskWarriorDbState::DataFields &fields)
        : fields(fields)
    {
    }

    void setField(const SplitString &split_string)
    {
        using handler_t = void (StatResponseSetters::*)(const SplitString &);
        const std::unordered_map<QString, handler_t> kTagToHandler = {
            { "Total", &StatResponseSetters::handleTotal },
            { "Undo", &StatResponseSetters::handleUndo },
            { "Sync", &StatResponseSetters::handleSync },
        };

        const auto it = kTagToHandler.find(split_string.key);
        if (it != kTagToHandler.end()) {
            const handler_t handler = it->second;
            (this->*handler)(split_string);
        }
    }

    [[nodiscard]]
    bool areAllFieldsParsed() const
    {
        return 3 == fields_parsed;
    }

  private:
    int fields_parsed{ 0 };
    TaskWarriorDbState::DataFields &fields;

    void handleTotal(const SplitString &str)
    {
        // Total                      35
        if (auto opt = getUint(str.value)) {
            ++fields_parsed;
            fields.fieldTotal = *opt;
        }
    }
    void handleUndo(const SplitString &str)
    {
        // Undo transactions          166
        if (auto opt = findFinalNumber(str.value)) {
            ++fields_parsed;
            fields.fieldUndo = *opt;
        }
    }
    void handleSync(const SplitString &str)
    {
        // Sync backlog transactions  645
        if (auto opt = findFinalNumber(str.value)) {
            ++fields_parsed;
            fields.fieldBacklog = *opt;
        }
    }

    [[nodiscard]]
    std::optional<uint> getUint(const QString &val) const
    {
        bool ok = false;
        uint res = val.toUInt(&ok);
        if (ok) {
            return res;
        }
        return std::nullopt;
    }

    [[nodiscard]]
    std::optional<uint> findFinalNumber(const QString &value_part) const
    {
        const QRegularExpression re("\\s*(\\d+)$");
        const QRegularExpressionMatch match = re.match(value_part);

        if (match.hasMatch()) {
            return getUint(match.captured(1));
        }
        return std::nullopt;
    }
};

} // namespace

DetailedTaskInfo::DetailedTaskInfo(QString task_id)
    : task_id(std::move(task_id))
    , description(&formatDescription)
    , project(&formatProject)
    , tags(&formatTags)
    , sched(&formatDateTime<ETaskDateTimeRole::Sched>)
    , due(&formatDateTime<ETaskDateTimeRole::Due>)
    , wait(&formatDateTime<ETaskDateTimeRole::Wait>)
    , priority(&formatPriority, Priority::Unset)
    // active has dedicated start/stop commands, it should not be formatted for
    // cmd
    , active("", false)
    // we do not pass reccurency_period to cmd yet
    , recurrency_period("", {})
{
}

DetailedTaskInfo::DetailedTaskInfo(int task_id)
    : DetailedTaskInfo(QStringLiteral("%1").arg(task_id))
{
}

DetailedTaskInfo::DetailedTaskInfo()
    : DetailedTaskInfo("")
{
}

DetailedTaskInfo::Priority
DetailedTaskInfo::priorityFromString(const QString &p)
{
    if (p == "L") {
        return Priority::L;
    }
    if (p == "M") {
        return Priority::M;
    }
    if (p == "H") {
        return Priority::H;
    }
    return Priority::Unset;
}

void DetailedTaskInfo::updateFrom(const DetailedTaskInfo &other)
{
    auto myProps = asTuple();
    const auto otherProps = other.asTuple();

    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        auto updater = [](auto &left, const auto &right) {
            if (left.get() != right.get()) {
                left = right;
            }
        };
        (updater(std::get<Is>(myProps), std::get<Is>(otherProps)), ...);
    }(std::make_index_sequence<std::tuple_size_v<decltype(myProps)>>{});
}

bool DetailedTaskInfo::execAddNewTask(const TaskWarriorExecutor &executor)
{
    const auto exec_res = executor.execTaskProgramWithDefaults(
        QStringList{ "add" } << getAddModifyCmdArgsFieldsRepresentation());
    if (!exec_res) {
        return false;
    }
    SetPropertiesNotChanged(true, TASK_PROPERTIES_LIST);
    return true;
}

bool DetailedTaskInfo::execModifyExisting(const TaskWarriorExecutor &executor)
{
    if (task_id.isEmpty() || !isInteger(task_id)) {
        return false;
    }
    return SetPropertiesNotChanged(
        executor.execTaskProgramWithDefaults(
            QStringList{ "modify", task_id }
            << getAddModifyCmdArgsFieldsRepresentation()),
        TASK_PROPERTIES_LIST);
}

bool DetailedTaskInfo::execReadExisting(const TaskWarriorExecutor &executor)
{
    if (task_id.isEmpty() || !isInteger(task_id)) {
        return false;
    }
    const auto exec_res = executor.execTaskProgramWithDefaults(
        QStringList{ "information", task_id });
    if (!exec_res) {
        return false;
    }
    QStringList stdOut = exec_res.getStdout();
    // Ensure we do not have empty lines.
    stdOut.removeAll("");
    if (!isTaskSentData(stdOut)) {
        return false;
    }

    enum class MultilineDescriptionStatus : std::uint8_t {
        NotStarted,
        InProgress
    };
    MultilineDescriptionStatus description_status{
        MultilineDescriptionStatus::NotStarted
    };

    // By default task is not reccurent and it is NOT indicated on full read.
    // If it is reccurent, it will have explicit period mentioned.
    recurrency_period.value.modify(
        [](RecurrentInstancePeriod &period) { period.setNonRecurrent(); });

    InformationResponseSetters setters(*this);
    std::for_each(
        std::next(stdOut.cbegin(), kHeadersSize),
        std::prev(stdOut.cend(), kFooterSize),
        [this, &description_status, &setters](const auto &whole_line) {
            // If string begins with spaces, split_string object is
            // invalid.
            const SplitString split_string(whole_line);
            if (!split_string.isValid()) {
                if (description_status ==
                    MultilineDescriptionStatus::InProgress) {
                    description =
                        description.get() + "\n" + whole_line.trimmed();
                }
                return;
            }
            if (split_string.key == "Description") {
                const auto descr1 = split_string.value.trimmed();
                if (!descr1.isEmpty()) {
                    description = descr1;
                    description_status = MultilineDescriptionStatus::InProgress;
                }
                return;
            }
            description_status = MultilineDescriptionStatus::NotStarted;
            setters.setField(split_string);
        });

    // After reading those are "not modified".
    SetPropertiesNotChanged(true, TASK_PROPERTIES_LIST);
    markFullRead();
    return true;
}

bool DetailedTaskInfo::isFullRead() const
{
    return dataState == ReadAs::FullRead &&
           recurrency_period.get().isFullyRead();
}

BatchTasksManager::BatchTasksManager(const QList<DetailedTaskInfo> &tasks)
    : BatchTasksManager(tasksListToIds(tasks, kTaskIdShortGetter))
{
}

BatchTasksManager::BatchTasksManager(QStringList tasks)
    : m_tasks_ids(std::move(tasks))
{
    const auto new_end = std::remove_if(
        m_tasks_ids.begin(), m_tasks_ids.end(), [](const QString &task_id) {
            return task_id.isEmpty() || !isInteger(task_id);
        });
    m_tasks_ids.erase(new_end, m_tasks_ids.end());
}

bool BatchTasksManager::execDeleteTask(
    const TaskWarriorExecutor &executor) const
{
    return execVerb("delete", executor);
}

bool BatchTasksManager::execStartTask(const TaskWarriorExecutor &executor) const
{
    return execVerb("start", executor);
}

bool BatchTasksManager::execStopTask(const TaskWarriorExecutor &executor) const
{
    return execVerb("stop", executor);
}

bool BatchTasksManager::execDoneTask(const TaskWarriorExecutor &executor) const
{
    return execVerb("done", executor);
}

bool BatchTasksManager::execWaitTask(const QDateTime &datetime,
                                     const TaskWarriorExecutor &executor) const
{
    const TaskDateTime<ETaskDateTimeRole::Wait> wait(datetime);
    return execVerb("modify", executor, formatDateTime(wait));
}

bool BatchTasksManager::execVerb(const QString &verb,
                                 const TaskWarriorExecutor &executor,
                                 const QString &after_ids) const
{
    if (m_tasks_ids.empty()) {
        return true;
    }
    return executor.execTaskProgramWithDefaults(
        { { verb, m_tasks_ids.join(','), after_ids } });
}

QStringList DetailedTaskInfo::getAddModifyCmdArgsFieldsRepresentation() const
{
    QStringList result;
    AppendPropertiesToCmdList(result, TASK_PROPERTIES_LIST);
    return result;
}

TaskWarriorDbState::Optional
TaskWarriorDbState::readCurrent(const TaskWarriorExecutor &executor)
{
    static const QStringList cmd = { "stat" };
    const auto exec_res = executor.execTaskProgramWithDefaults(cmd);
    if (!exec_res) {
        return std::nullopt;
    }
    QStringList stdOut = exec_res.getStdout();
    // Ensure we do not have empty lines.
    stdOut.removeAll("");
    // Note, we do not have footer here.
    if (stdOut.size() <= kHeadersSize) {
        return std::nullopt;
    }
    DataFields fields;
    StatResponseSetters setters(fields);
    std::for_each(std::next(stdOut.cbegin(), kHeadersSize), stdOut.cend(),
                  [&setters](const auto &whole_line) {
                      const SplitString split_string(whole_line);
                      if (!split_string.isValid()) {
                          return;
                      }
                      setters.setField(split_string);
                  });
    if (!setters.areAllFieldsParsed()) {
        return std::nullopt;
    }
    return TaskWarriorDbState(fields);
}

TaskWarriorDbState TaskWarriorDbState::invalidState()
{
    static const TaskWarriorDbState::DataFields invalid_state{
        std::numeric_limits<uint>::max(), std::numeric_limits<uint>::max(),
        std::numeric_limits<uint>::max()
    };
    return TaskWarriorDbState(invalid_state);
}
