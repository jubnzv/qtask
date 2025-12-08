#include "task.hpp"

#include "date_time_parser.hpp"
#include "qtutil.hpp"
#include "split_string.hpp"
#include "taskwarriorexecutor.hpp"

#include <QDateTime>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QStringLiteral>
#include <QVariant>
#include <qnamespace.h>
#include <qtypes.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace
{

// Escapes tag field, it should follow some rules, like tag cannot start
// by numeric digit.
QString escapeTag(QString cleanedTag)
{
    cleanedTag.replace(QRegularExpression(R"(\W)"), QString("_"));
    cleanedTag.replace(QRegularExpression(R"(_+)"), QString("_"));
    if (!cleanedTag.isEmpty() && cleanedTag.at(0).digitValue() != -1) {
        // Tag cannot start with digit or everything is going bad...
        cleanedTag = "T_" + cleanedTag;
    }
    return cleanedTag;
}

QStringList escapeTags(const QStringList &tags, QChar symbol)
{
    QStringList result;
    for (auto const &t : tags) {
        if (!t.isEmpty()) {
            const QString t_escpaed = escapeTag(t);
            if (!t_escpaed.isEmpty()) {
                result << QString{ "%2%1" }.arg(t_escpaed).arg(symbol);
            }
        }
    }

    return result;
}

auto escapeAddTags(const QStringList &tags) { return escapeTags(tags, '+'); }

auto escapeDelTags(const QStringList &tags) { return escapeTags(tags, '-'); }

QString escapeDescription(QString descr)
{
    // Single \ sign is missing, need to double it.
    descr.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    return descr;
}

QString formatDateTimeOptional(const QString &format,
                               const DetailedTaskInfo::OptionalDateTime &dt)
{
    static const QDateTime null_dt;
    return format.arg(dt.value_or(null_dt).toString(Qt::ISODate));
}

QString formatWaitOptional(const DetailedTaskInfo::OptionalDateTime &dt)
{
    return formatDateTimeOptional("wait:%1", dt);
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

QString formatPriority(const DetailedTaskInfo::Priority pri)
{
    if (pri != DetailedTaskInfo::Priority::Unset) {
        return QString{ "pri:'%1'" }.arg(priorityToChar(pri));
    } else {
        return QString{ "pri:''" };
    }
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
constexpr qsizetype kRowIndexOfDividers = 0;
constexpr qsizetype kHeadersSize = 2;
constexpr qsizetype kFooterSize = 1;

// returns true if task outputed something except header and footer.
bool isTaskSentData(const QStringList &task_output)
{
    // Empty lines are expected to be removed at all for this function to work.
    constexpr qsizetype kRowsAmountWhenEmptyResponse =
        kHeadersSize + kFooterSize;
    return task_output.size() > kRowsAmountWhenEmptyResponse;
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
            { "Project", &InformationResponseSetters::handleProject },
            { "Priority", &InformationResponseSetters::handlePriority },
            { "Tags", &InformationResponseSetters::handleTags },
            { "Start", &InformationResponseSetters::handleStart },
            { "Waiting", &InformationResponseSetters::handleWaiting },
            { "Scheduled", &InformationResponseSetters::handleScheduled },
            { "Due", &InformationResponseSetters::handleDue },
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
        task.wait = parser.parseDateTimeString(line.value);
    }
    void handleScheduled(const SplitString &line)
    {
        static const DateTimeParser parser{ 2, 0, 1 };
        task.sched = parser.parseDateTimeString(line.value);
    }
    void handleDue(const SplitString &line)
    {
        static const DateTimeParser parser{ 2, 0, 1 };
        task.due = parser.parseDateTimeString(line.value);
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

    bool areAllFieldsParsed() const { return 3 == fields_parsed; }

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

    std::optional<uint> getUint(const QString &val)
    {
        bool ok = false;
        uint res = val.toUInt(&ok);
        if (ok) {
            return res;
        }
        return std::nullopt;
    }

    std::optional<uint> findFinalNumber(const QString &value_part)
    {
        QRegularExpression re("\\s*(\\d+)$");
        QRegularExpressionMatch match = re.match(value_part);

        if (match.hasMatch()) {
            return getUint(match.captured(1));
        }
    }
};

} // namespace

#define TASK_PROPERTIES_LIST \
    priority, project, tags, removed_tags, sched, due, wait, description

DetailedTaskInfo::DetailedTaskInfo(QString task_id)
    : task_id(std::move(task_id))
    , description(&escapeDescription)
    , project("project:'%1'")
    , tags(&escapeAddTags)
    , removed_tags(&escapeDelTags)
    , sched(
          [](const auto &dt) { return formatDateTimeOptional("sched:%1", dt); })
    , due([](const auto &dt) { return formatDateTimeOptional("due:%1", dt); })
    , wait([](const auto &dt) { return formatWaitOptional(dt); })
    , priority(&formatPriority, Priority::Unset)
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
                        description.get() + "\n" + whole_line.simplified();
                }
                return;
            }
            if (split_string.key == "Description") {
                const auto descr1 = split_string.value.simplified();
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
    dataState = ReadAs::FullRead;
    return true;
}

bool DetailedTaskInfo::isFullRead() const
{
    return dataState == ReadAs::FullRead;
}

BatchTasksManager::BatchTasksManager(const QList<DetailedTaskInfo> &tasks)
    : BatchTasksManager([&tasks] {
        QStringList ids;
        std::transform(tasks.begin(), tasks.end(), std::back_inserter(ids),
                       [](const auto &task) { return task.task_id; });
        return ids;
    }())
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
    return execVerb("modify", executor, formatWaitOptional(datetime));
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

FilteredTasksListReader::FilteredTasksListReader(AllAtOnceKeywordsFinder filter)
    : tasks{}
    , m_filter(std::move(filter))
{
}

bool FilteredTasksListReader::readTaskList(const TaskWarriorExecutor &executor)
{
    constexpr qsizetype kExpectedColumnsCount = 6;
    auto cmd = QStringList{
        // clang-format off
                         "rc.report.minimal.columns=id,start.active,project,priority,scheduled,due,description",
        // clang-format on
        "rc.report.minimal.labels=',|,|,|,|,|,|'",
        "rc.report.minimal.sort=urgency-",
        "rc.print.empty.columns=yes",
        "rc.dateformat=Y-M-DTH:N:S",
        "+PENDING",
        "minimal",
    };
    if (m_filter.getIds().has_value()) {
        cmd << *m_filter.getIds();
    }
    const auto res = executor.execTaskProgramWithDefaults(cmd);
    if (!res) {
        return false;
    }
    QStringList stdOut = res.getStdout();
    stdOut.removeAll("");
    if (!isTaskSentData(stdOut)) {
        return false;
    }
    tasks.clear();

    // Get positions from the labels string.
    const auto opt_positions = findColumnPositions<kExpectedColumnsCount>(
        stdOut.at(kRowIndexOfDividers));
    if (!opt_positions) {
        return false;
    }
    const auto &positions = *opt_positions;

    bool found_annotation = false;
    for (auto line_it = std::next(stdOut.begin(), kHeadersSize),
              last_it = std::prev(stdOut.end(), kFooterSize);
         line_it != last_it; ++line_it) {
        const auto &line = *line_it;

        if (line.size() < positions[3]) {
            continue;
        }

        // TODO: it could be simplified too, probably.
        DetailedTaskInfo task(line.mid(0, positions[0]).simplified());
        if (!isInteger(task.task_id)) {
            // It's probably a continuation of the multiline description or
            // an annotation from the previous task.
            if (tasks.isEmpty() || found_annotation) {
                continue;
            }
            const QString desc_line =
                line.right(line.size() - positions[3]).simplified();
            if (desc_line.isEmpty()) {
                continue;
            }
            // The annotations always start with a timestamp. And they
            // always follows the description.
            const QString first_word =
                line.section(' ', 0, 0, QString::SectionSkipEmpty).simplified();
            if (QDateTime::fromString(first_word, Qt::ISODate).isValid()) {
                // We won't handle the annotations at all.
                found_annotation = true;
                continue;
            }

            tasks.back().description =
                tasks.back().description.get() + '\n' + desc_line;
            continue;
        }

        found_annotation = false;

        const QString start_mark =
            line.mid(positions[0], positions[1] - positions[0]).simplified();
        task.active = start_mark.contains('*');
        task.project =
            line.mid(positions[1], positions[2] - positions[1]).simplified();
        task.priority = DetailedTaskInfo::priorityFromString(
            line.mid(positions[2], positions[3] - positions[2]).simplified());
        auto sched = QDateTime::fromString(
            line.mid(positions[3], positions[4] - positions[3]).simplified(),
            Qt::ISODate);
        if (sched.isValid()) {
            task.sched = sched;
        }
        auto due = QDateTime::fromString(
            line.mid(positions[4], positions[5] - positions[4]).simplified(),
            Qt::ISODate);
        if (due.isValid()) {
            task.due = due;
        }
        task.description = line.right(line.size() - positions[5]).simplified();
        tasks.emplace_back(std::move(task));
    }

    return true;
}

AllAtOnceKeywordsFinder::AllAtOnceKeywordsFinder(QStringList keywords)
    : m_user_keywords(std::move(keywords))
{
}

bool AllAtOnceKeywordsFinder::readIds(const TaskWarriorExecutor &executor)
{
    const auto resp = executor.execTaskProgramWithDefaults(QStringList("ids")
                                                           << m_user_keywords);

    m_ids = std::nullopt;
    if (resp) {
        const auto &stdOut = resp.getStdout();

        // Examples:
        // task ids Bank
        // 1-2
        // task ids 1 2 3 4 5 6 7 8 9 111
        // 1-6
        // task ids 2 5 1 4
        // 1-2 4-5

        if (stdOut.size() == 1) {
            m_ids = stdOut.first();
            return true;
        }
        std::cerr << "Unexpected result of ids command: "
                  << stdOut.join("\n").toStdString() << std::endl;
    }
    return false;
}

std::optional<QList<RecurringTaskTemplate>>
RecurringTaskTemplate::readAll(const TaskWarriorExecutor &executor)
{
    constexpr qsizetype kExpectedColumnsCount = 3;
    const auto response = executor.execTaskProgramWithDefaults(QStringList{
        "recurring_full",
        "rc.report.recurring_full.columns=id,recur,project,description",
        "rc.report.recurring_full.labels=',|,|,|'",
        "status:Recurring",
    });

    const auto &tasks_strs = response.getStdout();
    if (!response) {
        return std::nullopt;
    }
    if (!isTaskSentData(tasks_strs)) {
        return QList<RecurringTaskTemplate>{}; // no tasks
    }

    // Get positions from the labels string
    // id created mod status recur wait due project description mask
    const auto opt_positions = findColumnPositions<kExpectedColumnsCount>(
        tasks_strs.at(kRowIndexOfDividers));
    if (!opt_positions) {
        return std::nullopt;
    }
    const auto &positions = *opt_positions;

    QList<RecurringTaskTemplate> out_tasks;
    for (auto line_it = std::next(tasks_strs.begin(), kHeadersSize),
              last_it = std::prev(tasks_strs.end(), kFooterSize);
         line_it != last_it; ++line_it) {
        const auto &line = *line_it;

        RecurringTaskTemplate task;
        task.uuid = line.mid(0, positions[0]).simplified();
        task.period =
            line.mid(positions[0], positions[1] - positions[0]).simplified();
        task.project =
            line.mid(positions[1], positions[2] - positions[1]).simplified();
        task.description = line.right(line.size() - positions[2]).simplified();
        out_tasks.push_back(task);
    }

    return out_tasks;
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
