#include "taskwarrior.hpp"

#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>
#include <qassert.h>
#include <qlogging.h>
#include <qnamespace.h>
#include <qtversionchecks.h>
#include <qtypes.h>

#include "configmanager.hpp"
#include "qtutil.hpp"
#include "task.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <optional>
#include <unordered_map>
#include <utility>
#include <variant>

namespace
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
constexpr auto kSplitBehaviour = Qt::SkipEmptyParts;
#else
constexpr auto kSplitBehaviour = QString::SkipEmptyParts;
#endif // QT_VERSION_CHECK

/// @brief Splits space separated string into QStringList.
/// @returns QStringList, each element is part of string between spaces or line
/// ends.
[[nodiscard]] QStringList
splitSpaceSeparatedString(const QString &args_as_space_separated_string)
{
    static const QRegularExpression kWhitespaceRx("\\s+");
    return args_as_space_separated_string.split(kWhitespaceRx, kSplitBehaviour);
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

constexpr qsizetype kRowIndexOfDividers = 0;
constexpr qsizetype kHeadersSize = 2;

/// @brief Takes 1st word as key and everything else - as value.
/// Key must be aligned to begin of the string, any amount of space/tabs can
/// separated key/value.
/// @note if string begins with spaces it will be invalid object.
struct SplitString {
    QString key;
    QString value;

    explicit SplitString(const QString &line)
    {
        // (.*) ^ - string's begin
        // (\S+) - Group 1: non-space symbols (key / left word)
        // \s* - 0 or more space characters - delimeter
        // (.*) - Group 2 - all other symbols till the string end
        static const QRegularExpression kSplitRx("^(\\S+)\\s*(.*)");

        if (!line.isEmpty()) {
            const QRegularExpressionMatch match = kSplitRx.match(line);
            if (match.hasMatch()) {
                key = match.captured(1).simplified();
                value = match.captured(2).simplified();
            }
        }
    }

    [[nodiscard]]
    bool isValid() const
    {
        return !key.isEmpty();
    }
};

QString formatDateTime(const QDateTime &datetime)
{
    return datetime.toString(Qt::ISODate);
}

/// @brief Parser of date/time in "task" output.
struct DateTimeParser {
    qsizetype expected_lexems_count;
    qsizetype date_lexem_index;
    qsizetype time_lexem_index;

    [[nodiscard]]
    AbstractTask::OptionalDateTime
    parseDateTimeString(const QString &line_of_lexems) const
    {
        Q_ASSERT(date_lexem_index < expected_lexems_count);
        Q_ASSERT(time_lexem_index < expected_lexems_count);

        const auto lexems = splitSpaceSeparatedString(line_of_lexems);
        if (lexems.size() != expected_lexems_count) {
            return std::nullopt;
        }
        return composeDateTime(lexems.at(date_lexem_index),
                               lexems.at(time_lexem_index));
    }

  private:
    [[nodiscard]]
    static AbstractTask::OptionalDateTime composeDateTime(const QString &date,
                                                          const QString &time)
    {
        auto dt = QDateTime::fromString(QString{ "%1T%2" }.arg(date, time),
                                        Qt::ISODate);
        if (dt.isValid()) {
            return dt;
        }
        return std::nullopt;
    }
};

/// @brief Error code and error message of executing the program.
struct TExecError {
    int code;
    QString message;
};

/// @brief It is resulting error code or stdout of calling external binary.
struct TExecResult {
    std::variant<TExecError, QStringList> exec_result_value;
    static constexpr int kSuccessCode = 0;

    [[nodiscard]]
    const TExecError &getError() const
    {
        static const TExecError kNoError{ kSuccessCode, {} };

        if (const TExecError *error =
                std::get_if<TExecError>(&exec_result_value)) {
            return *error;
        }
        return kNoError;
    }

    [[nodiscard]]
    const QStringList &getStdout() const
    {
        static const QStringList kNoStdout;

        if (const QStringList *stdout_lines =
                std::get_if<QStringList>(&exec_result_value)) {
            return *stdout_lines;
        }
        return kNoStdout;
    }

    /// @brief This object evaluates to true in bool context if it has no error.
    /// @returns true if it was NO error.
    [[nodiscard]]
    operator bool() const // NOLINT
    {
        return getError().code == kSuccessCode;
    }
};

/// @brief Executes configured task and @returns TExecResult.
TExecResult execProgram(const QString &binary, const QStringList &all_params)
{
    constexpr int kStartDelayMs = 1000;
    constexpr int kFinishDelayMs = 30000;

    QProcess proc;
    proc.start(binary, all_params);

    if (!proc.waitForStarted(kStartDelayMs)) {
        return { TExecError{
            -1, QString("Failed to start: [ %1 ].").arg(proc.errorString()) } };
    }

    if (!proc.waitForFinished(kFinishDelayMs)) {
        return { TExecError{
            -1,
            QString("Execution timeout after %1ms.").arg(kFinishDelayMs) } };
    }

    const int exitCode = proc.exitCode();
    if (proc.exitStatus() != QProcess::NormalExit || exitCode != 0) {
        return { TExecError{
            exitCode != 0 ? exitCode : -1,
            QString::fromLocal8Bit(proc.readAllStandardError()) } };
    }

    static const QRegularExpression kSplitter("[\r\n]");
    return TExecResult{ QString::fromLocal8Bit(proc.readAllStandardOutput())
                            .split(kSplitter, kSplitBehaviour) };
}

/// @brief Executes configured "task" program and @returns TExecResult.
TExecResult execTaskProgram(const QStringList &all_params)
{
    const auto &binary = ConfigManager::config().getTaskBin();
    qDebug() << binary << " " << all_params;
    auto res = execProgram(binary, all_params);
    qDebug() << res;
    if (!res) {
        std::cerr << "Error executing " << binary.toStdString() << "\n";
        std::cerr << "Code: " << res.getError().code << ". "
                  << res.getError().message.toStdString() << std::endl;
    }
    return res;
}

/// @brief Executes configured task with defaults required parameters and
/// @returns TExecResult.
TExecResult execTaskProgramWithDefaults(const QStringList &all_params)
{
    QStringList args{ "rc.gc=off", "rc.confirmation=off", "rc.bulk=0",
                      "rc.defaultwidth=0" };
    args << all_params;
    return execTaskProgram(args);
}

} // namespace

Taskwarrior::Taskwarrior()
    : m_actions_counter(0ull)
    , m_task_version(QVariant{})
{
}

Taskwarrior::~Taskwarrior() = default;

bool Taskwarrior::init()
{
    const auto version_res = execTaskProgram({ "version" });
    const auto &ver_strings = version_res.getStdout();
    if (!version_res || ver_strings.size() < 1) {
        return false;
    }

    const auto task_version =
        splitSpaceSeparatedString(ver_strings.at(0)).at(1);
    if (task_version.isEmpty()) {
        return false;
    }
    m_task_version = { task_version };

    return execTaskProgram({ "rc.gc=on" });
}

bool Taskwarrior::addTask(const Task &task)
{
    if (execTaskProgramWithDefaults(QStringList{ "add" }
                                    << task.getCmdArgs())) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::startTask(const QString &id)
{
    if (execTaskProgramWithDefaults({ "start", id })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::stopTask(const QString &id)
{
    if (execTaskProgramWithDefaults({ "stop", id })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::editTask(const Task &task)
{
    if (execTaskProgramWithDefaults(QStringList{ task.uuid, "modify" }
                                    << task.getCmdArgs())) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::setPriority(const QString &id, Task::Priority p)
{
    if (execTaskProgramWithDefaults(
            { id, "modify",
              QString{ "pri:'%1'" }.arg(Task::priorityToString(p)) })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

std::optional<Task> Taskwarrior::getTask(const QString &id) const
{
    using TaskProcessor = std::function<void(const SplitString &, Task &)>;
    static const std::unordered_map<QString, TaskProcessor>
        kSplitLineProcessors = {
            { "ID",
              [](const auto &line, auto &task) { task.uuid = line.value; } },
            { "Project",
              [](const auto &line, auto &task) { task.project = line.value; } },
            { "Priority",
              [](const auto &line, auto &task) {
                  task.priority = Task::priorityFromString(line.value);
              } },
            { "Tags",
              [](const auto &line, auto &task) {
                  task.tags = splitSpaceSeparatedString(line.value);
              } },
            { "Start", [](const auto &, auto &task) { task.active = true; } },
            { "Waiting",
              [](const auto &line, auto &task) {
                  // Waiting until 2025-11-04 00:00:00
                  // so "until" becomes 0th token of SplitString::value
                  static const DateTimeParser parser{ 3, 1, 2 };
                  task.wait = parser.parseDateTimeString(line.value);
              } },
            { "Scheduled",
              [](const auto &line, auto &task) {
                  static const DateTimeParser parser{ 2, 0, 1 };
                  task.sched = parser.parseDateTimeString(line.value);
              } },
            { "Due",
              [](const auto &line, auto &task) {
                  static const DateTimeParser parser{ 2, 0, 1 };
                  task.due = parser.parseDateTimeString(line.value);
              } },
        };

    enum class MultilineDescriptionStatus : std::uint8_t {
        NotStarted,
        InProgress
    };
    MultilineDescriptionStatus description_status{
        MultilineDescriptionStatus::NotStarted
    };

    const auto info_res = execTaskProgramWithDefaults({ id, "information" });
    if (!info_res) {
        return std::nullopt;
    }
    const auto &info_out = info_res.getStdout();
    if (info_out.size() < kHeadersSize + 1) {
        return std::nullopt; // not found
    }

    Task task;
    std::for_each(
        std::next(info_out.cbegin(), kHeadersSize), info_out.cend(),
        [&task, &description_status](const auto &whole_line) {
            // If string begins with spaces, split_string object is invalid.
            const SplitString split_string(whole_line);
            if (!split_string.isValid()) {
                if (description_status ==
                    MultilineDescriptionStatus::InProgress) {
                    task.description += '\n' + whole_line.simplified();
                }
                return;
            }
            if (split_string.key == "Description") {
                const auto descr1 = split_string.value.simplified();
                if (!descr1.isEmpty()) {
                    task.description = descr1;
                    description_status = MultilineDescriptionStatus::InProgress;
                }
                return;
            }
            description_status = MultilineDescriptionStatus::NotStarted;

            const auto it = kSplitLineProcessors.find(split_string.key);
            if (it != kSplitLineProcessors.end()) {
                it->second(split_string, task);
                return;
            }
        });

    return task;
}

std::optional<QList<Task>> Taskwarrior::getTasks() const
{
    constexpr qsizetype kExpectedColumnsCount = 6;
    const auto response = execTaskProgramWithDefaults(
        QStringList{
            // clang-format off
            "rc.report.minimal.columns=id,start.active,project,priority,scheduled,due,description",
            // clang-format on
            "rc.report.minimal.labels=',|,|,|,|,|,|'",
            "rc.report.minimal.sort=urgency-",
            "rc.print.empty.columns=yes",
            "rc.dateformat=Y-M-DTH:N:S",
            "+PENDING",
            "minimal",
        }
        << m_filter);

    const auto &tasks_strs = response.getStdout();
    if (!response || tasks_strs.size() < kHeadersSize + 1) {
        return std::nullopt;
    }

    // Get positions from the labels string.
    const auto opt_positions = findColumnPositions<kExpectedColumnsCount>(
        tasks_strs.at(kRowIndexOfDividers));
    if (!opt_positions) {
        return std::nullopt;
    }
    const auto &positions = *opt_positions;

    QList<Task> tasks_result;

    bool found_annotation = false;
    for (auto line_it = std::next(tasks_strs.begin(), kHeadersSize);
         line_it != tasks_strs.end(); ++line_it) {
        const auto &line = *line_it;

        if (line.size() < positions[3]) {
            continue;
        }

        // TODO: it could be simplified too, probably.
        Task task;
        task.uuid = line.mid(0, positions[0]).simplified();

        if (!isInteger(task.uuid)) {
            // It's probably a continuation of the multiline description or
            // an annotation from the previous task.
            if (tasks_result.isEmpty() || found_annotation) {
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

            tasks_result.back().description += '\n' + desc_line;
            continue;
        }

        found_annotation = false;

        const QString start_mark =
            line.mid(positions[0], positions[1] - positions[0]).simplified();
        task.active = start_mark.contains('*');
        task.project =
            line.mid(positions[1], positions[2] - positions[1]).simplified();
        task.priority = Task::priorityFromString(
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
        tasks_result.emplace_back(std::move(task));
    }

    return tasks_result;
}

std::optional<QList<RecurringTask>> Taskwarrior::getRecurringTasks() const
{
    constexpr qsizetype kExpectedColumnsCount = 3;
    const auto response = execTaskProgramWithDefaults(QStringList{
        "recurring_full",
        "rc.report.recurring_full.columns=id,recur,project,description",
        "rc.report.recurring_full.labels=',|,|,|'",
        "status:Recurring",
    });

    const auto &tasks_strs = response.getStdout();
    if (!response) {
        return std::nullopt;
    }
    if (tasks_strs.size() < kHeadersSize + 1) {
        return QList<RecurringTask>{}; // no tasks
    }

    // Get positions from the labels string
    // id created mod status recur wait due project description mask
    const auto opt_positions = findColumnPositions<kExpectedColumnsCount>(
        response.getStdout().at(kRowIndexOfDividers));
    if (!opt_positions) {
        return std::nullopt;
    }
    const auto &positions = *opt_positions;

    QList<RecurringTask> out_tasks;
    for (auto line_it = std::next(tasks_strs.begin(), kHeadersSize);
         line_it != tasks_strs.end(); ++line_it) {
        const auto &line = *line_it;

        RecurringTask task;
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

bool Taskwarrior::deleteTask(const QString &id)
{
    if (execTaskProgramWithDefaults({ "delete", id })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::deleteTask(const QStringList &ids)
{
    if (ids.isEmpty()) {
        return true;
    }
    if (execTaskProgramWithDefaults({ "delete", ids.join(',') })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::setTaskDone(const QString &id)
{
    if (execTaskProgramWithDefaults({ "done", id })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::setTaskDone(const QStringList &ids)
{
    if (ids.isEmpty()) {
        return true;
    }
    if (execTaskProgramWithDefaults({ "done", ids.join(',') })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::waitTask(const QString &id, const QDateTime &datetime)
{
    if (execTaskProgramWithDefaults(
            { "modify", id,
              QString("wait:%1").arg(formatDateTime(datetime)) })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::waitTask(const QStringList &ids, const QDateTime &datetime)
{
    if (ids.isEmpty()) {
        return true;
    }
    if (execTaskProgramWithDefaults(
            { "modify", ids.join(','),
              QString("wait:%1").arg(formatDateTime(datetime)) })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::undoTask()
{
    if (m_actions_counter == 0) {
        return true;
    }
    if (execTaskProgramWithDefaults({ "undo" })) {
        --m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::applyFilter(const QStringList &filter)
{
    m_filter = filter;
    if (filter.isEmpty()) {
        return true;
    }
    if (execTaskProgramWithDefaults(QStringList{ "ids" }
                                    << m_filter)) { // test the new filter
        return true;
    }
    m_filter.clear();
    return false;
}

int Taskwarrior::directCmd(const QString &cmd)
{
    return execTaskProgramWithDefaults(splitSpaceSeparatedString(cmd))
        .getError()
        .code;
}
