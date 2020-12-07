#include "taskwarrior.hpp"

#include <QDateTime>
#include <QDebug>
#include <QList>
#include <QProcess>
#include <QString>
#include <QVariant>

#include "configmanager.hpp"
#include "task.hpp"

const QStringList Taskwarrior::s_args = { "rc.gc=off", "rc.confirmation=off",
                                          "rc.bulk=0", "rc.defaultwidth=0" };

Taskwarrior::Taskwarrior()
    : m_actions_counter(0ull)
    , m_task_version(QVariant{})
{
}

Taskwarrior::~Taskwarrior() {}

bool Taskwarrior::init()
{
    QByteArray out;
    if (!execCmd({ "version" }, out, false, false))
        return false;
    auto out_bytes = out.split('\n');
    if (out_bytes.size() < 2)
        return false;
    QString line = out_bytes[1];
    QString task_version = line.split(' ', Qt::SkipEmptyParts)[1];
    if (task_version.isEmpty())
        return false;
    m_task_version = { QString(task_version) };

    if (!execCmd({ "rc.gc=on" }, false, false))
        return false;

    return true;
}

bool Taskwarrior::addTask(const Task &task)
{
    if (execCmd(QStringList() << "add" << task.getCmdArgs())) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::startTask(const QString &id)
{
    if (execCmd({ "start", id })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::stopTask(const QString &id)
{
    if (execCmd({ "stop", id })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::editTask(const Task &task)
{
    if (execCmd(QStringList() << task.uuid << "modify" << task.getCmdArgs())) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::setPriority(const QString &id, Task::Priority p)
{
    const auto p_str = QString{ "pri:'%1'" }.arg(Task::priorityToString(p));
    if (execCmd({ id, "modify", p_str })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::getTask(const QString &id, Task &task)
{
    QByteArray out;
    auto args = QStringList() << id << "information";
    if (!execCmd(args, out))
        return false;

    auto out_bytes = out.split('\n');
    if (out_bytes.size() < 5)
        return false; // not found
    bool desc_done = false;
    for (size_t i = 3; i < out_bytes.size() - 3; ++i) {
        const QByteArray &bytes = out_bytes[i];
        if (bytes.isEmpty())
            continue;
        QString line(bytes);
        if (line.startsWith("ID")) {
            task.uuid = line.section(' ', 1).simplified();
            continue;
        }
        if (line.startsWith("Description")) {
            task.description = line.section(' ', 1).simplified();
            continue;
        }
        if (line.startsWith("Project")) {
            task.project = line.section(' ', 1).simplified();
            continue;
        }
        if (line.startsWith("Priority")) {
            task.priority =
                Task::priorityFromString(line.section(' ', 1).simplified());
            continue;
        }
        if (line.startsWith("Tags")) {
            auto tags_line = line.section(' ', 1).simplified();
            task.tags = tags_line.split(' ', Qt::SkipEmptyParts);
            continue;
        }
        if (line.startsWith("Start")) {
            task.active = true;
            continue;
        }
        if (line.startsWith("Waiting")) {
            const QStringList lexemes = line.split(' ', Qt::SkipEmptyParts);
            if (lexemes.size() != 4)
                continue;
            auto dt = QDateTime::fromString(
                QString{ "%1T%2" }.arg(lexemes[2], lexemes[3]), Qt::ISODate);
            if (dt.isValid())
                task.wait = dt;
            continue;
        }
        if (line.startsWith("Scheduled")) {
            const QStringList lexemes = line.split(' ', Qt::SkipEmptyParts);
            if (lexemes.size() != 3)
                continue;
            auto dt = QDateTime::fromString(
                QString{ "%1T%2" }.arg(lexemes[1], lexemes[2]), Qt::ISODate);
            if (dt.isValid())
                task.sched = dt;
            continue;
        }
        if (line.startsWith("Due")) {
            const QStringList lexemes = line.split(' ', Qt::SkipEmptyParts);
            if (lexemes.size() != 3)
                continue;
            auto dt = QDateTime::fromString(
                QString{ "%1T%2" }.arg(lexemes[1], lexemes[2]), Qt::ISODate);
            if (dt.isValid())
                task.due = dt;
            continue;
        } else {
            // Searching multiline description
            if (desc_done || line.startsWith("Status")) {
                desc_done = true;
                continue;
            }

            QString desc_line = line.section(' ', 1).simplified();
            if (desc_line.isEmpty())
                continue;

            QString first_word =
                line.section(' ', 0, 0, QString::SectionSkipEmpty).simplified();
            QDateTime dt = QDateTime::fromString(first_word, Qt::ISODate);
            if (dt.isValid()) {
                desc_done = true;
                continue;
            }

            task.description += '\n' + desc_line;
        }
    }

    return true;
}

bool Taskwarrior::getTasks(QList<Task> &tasks)
{
    QByteArray out;
    auto args = QStringList() << "rc.report.minimal.columns=id,start.active,"
                                 "project,priority,description"
                              << "rc.report.minimal.labels=',|,|,|,|'"
                              << "rc.report.minimal.sort=urgency-"
                              << "rc.print.empty.columns=yes"
                              << "+PENDING"
                              << "minimal";

    if (!execCmd(args, out, /*filter_enabled=*/true))
        return false;

    auto out_bytes = out.split('\n');
    if (out_bytes.size() < 6)
        return true; // no tasks

    // Get positions from the labels string
    QVector<int> positions;
    constexpr int columns_num = 4;
    for (int i = 0; i < out_bytes[1].size(); ++i) {
        const auto b = out_bytes[1][i];
        if (b == '|')
            positions.push_back(i);
    }
    if (positions.size() != columns_num) {
        return false;
    }

    bool found_annotation = false;
    for (size_t i = 3; i < out_bytes.size() - 3; ++i) {
        const QByteArray &bytes = out_bytes[i];
        if (bytes.isEmpty())
            continue;
        QString line(bytes);
        if (line.size() < positions[3])
            return false;

        Task task;

        task.uuid = line.mid(0, positions[0]).simplified();
        bool can_convert = false;
        (void)task.uuid.toInt(&can_convert);
        if (!can_convert) {
            // It's probably a continuation of the multiline description or an
            // annotation from the previous task.
            if ((line.size() < positions[3]) || tasks.isEmpty() ||
                found_annotation)
                continue;

            QString desc_line =
                line.right(line.size() - positions[3]).simplified();
            if (desc_line.isEmpty())
                continue;

            // The annotations always start with a timestamp. And they always
            // follows the description.
            QString first_word =
                line.section(' ', 0, 0, QString::SectionSkipEmpty).simplified();
            QDateTime dt = QDateTime::fromString(first_word, Qt::ISODate);
            if (dt.isValid()) {
                // We won't handle the annotations at all.
                found_annotation = true;
                continue;
            }

            tasks[tasks.size() - 1].description += '\n' + desc_line;
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
        task.description = line.right(line.size() - positions[3]).simplified();

        tasks.push_back(task);
    }

    return true;
}

bool Taskwarrior::getRecurringTasks(QList<RecurringTask> &out_tasks)
{
    QByteArray out;

    auto args = QStringList() << "recurring_full"
                              << "rc.report.recurring_full.columns=id,recur,"
                                 "project,description"
                              << "rc.report.recurring_full.labels=',|,|,|'"
                              << "status:Recurring";

    if (!execCmd(args, out))
        return false;

    auto out_bytes = out.split('\n');
    if (out_bytes.size() < 5)
        return true; // no tasks

    // Get positions from the labels string
    // id created mod status recur wait due project description mask
    QVector<int> positions;
    constexpr int columns_num = 3;
    for (int i = 0; i < out_bytes[2].size(); ++i) {
        const auto b = out_bytes[2][i];
        if (b == ' ')
            positions.push_back(i);
    }
    if (positions.size() != columns_num) {
        return false;
    }

    for (size_t i = 3; i < out_bytes.size() - 2; ++i) {
        const QByteArray &bytes = out_bytes[i];
        if (bytes.isEmpty())
            continue;
        QString line(bytes);

        RecurringTask task;
        task.uuid =
            line.section(' ', 0, 0, QString::SectionSkipEmpty).simplified();
        task.period =
            line.mid(positions[0], positions[1] - positions[0]).simplified();
        task.project =
            line.mid(positions[1], positions[2] - positions[1]).simplified();
        task.description = line.right(line.size() - positions[2]).simplified();

        out_tasks.push_back(task);
    }

    return true;
}

bool Taskwarrior::deleteTask(const QString &id)
{
    if (execCmd({ "delete", id })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::deleteTask(const QStringList &ids)
{
    if (ids.isEmpty())
        return true;
    if (execCmd({ "delete", ids.join(',') })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::setTaskDone(const QString &id)
{
    if (execCmd({ "done", id })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::setTaskDone(const QStringList &ids)
{
    if (ids.isEmpty())
        return true;
    if (execCmd({ "done", ids.join(',') })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::waitTask(const QString &id, const QDateTime &datetime)
{
    if (execCmd({ "modify", id,
                  QString("wait:%1").arg(formatDateTime(datetime)) })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::waitTask(const QStringList &ids, const QDateTime &datetime)
{
    if (ids.isEmpty())
        return true;
    if (execCmd({ "modify", ids.join(','),
                  QString("wait:%1").arg(formatDateTime(datetime)) })) {
        ++m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::undoTask()
{
    if (m_actions_counter == 0)
        return true;
    if (execCmd({ "undo" })) {
        --m_actions_counter;
        return true;
    }
    return false;
}

bool Taskwarrior::applyFilter(const QStringList &filter)
{
    m_filter = filter;
    if (filter.isEmpty())
        return true;
    if (execCmd({ "ids" }, /*filter_enabled=*/true)) { // test the new filter
        return true;
    }
    m_filter.clear();
    return false;
}

int Taskwarrior::directCmd(const QString &cmd)
{
    QProcess proc;
    int rc = -1;

    QStringList args = cmd.split(' ', Qt::SkipEmptyParts) << s_args;

    proc.start(ConfigManager::config()->getTaskBin(), args);
    if (proc.waitForStarted(1000)) {
        if (proc.waitForFinished() &&
            (proc.exitStatus() == QProcess::NormalExit))
            rc = proc.exitCode();
    }

    return rc;
}

bool Taskwarrior::execCmd(const QStringList &args, bool filter_enabled,
                          bool use_standard_args)
{
    QProcess proc;
    int rc = -1;

    QStringList real_args;
    if (filter_enabled)
        real_args << m_filter;
    if (use_standard_args)
        real_args << s_args;
    real_args << args;

    qDebug() << ConfigManager::config()->getTaskBin() << real_args;
    proc.start(ConfigManager::config()->getTaskBin(), real_args);

    if (proc.waitForStarted(1000)) {
        if (proc.waitForFinished(1000) &&
            (proc.exitStatus() == QProcess::NormalExit))
            rc = proc.exitCode();
    }

    return rc == 0;
}

bool Taskwarrior::execCmd(const QStringList &args, QByteArray &out,
                          bool filter_enabled, bool use_standard_args)
{
    QProcess proc;
    int rc = -1;

    QStringList real_args;
    if (filter_enabled)
        real_args << m_filter;
    if (use_standard_args)
        real_args << s_args;
    real_args << args;

    qDebug() << ConfigManager::config()->getTaskBin() << real_args;
    proc.start(ConfigManager::config()->getTaskBin(), real_args);

    if (proc.waitForStarted(1000)) {
        if (proc.waitForFinished(1000) &&
            (proc.exitStatus() == QProcess::NormalExit))
            rc = proc.exitCode();
    }

    if (rc != 0)
        return false;
    out = proc.readAllStandardOutput();
    return true;
}

QString Taskwarrior::formatDateTime(const QDateTime &datetime) const
{
    return datetime.toString(Qt::ISODate);
}
