#include "filteredtaskslistreader.hpp"
#include "allatoncekeywordsfinder.hpp"
#include "lambda_visitors.hpp"
#include "task.hpp"
#include "task_table_stencil.hpp"
#include "taskwarriorexecutor.hpp"

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <qnamespace.h>

#include <utility>
#include <variant>

namespace
{
template <typename taProperty, typename taValue>
void LoadTaskField(taProperty &field, taValue value)
{
    field = std::move(value);
    field.value.setNotModified();
}

template <typename taProperty>
void LoadTaskDate(taProperty &field, const QString &date)
{
    auto dt = QDateTime::fromString(date.trimmed(), Qt::ISODate);
    if (dt.isValid()) {
        LoadTaskField(field, std::move(dt));
    }
}

} // namespace

FilteredTasksListReader::FilteredTasksListReader(AllAtOnceKeywordsFinder filter)
    : tasks{}
    , m_filter(std::move(filter))
{
}

#define SKIP_CONTINUATION                \
    if (m != Mode::FirstLineOfNewRecord) \
    return

const FilteredTasksListReader::ColumnsSchema &
FilteredTasksListReader::getSchema() const
{
    using Task = data_type;
    using Mode = ColumnDescriptor::LineMode;

    static const ColumnsSchema schema = {
        {
            "id",
            [](const QString &v, Task &t, Mode m) {
                SKIP_CONTINUATION;
                t.task_id = v.trimmed();
            },
        },
        {
            "start.active",
            [](const QString &v, Task &t, Mode m) {
                SKIP_CONTINUATION;
                LoadTaskField(t.active, !v.trimmed().isEmpty());
            },
        },
        {
            "project",
            [](const QString &v, Task &t, Mode m) {
                SKIP_CONTINUATION;
                LoadTaskField(t.project, v.trimmed());
            },
        },
        {
            "priority",
            [](const QString &v, Task &t, Mode m) {
                SKIP_CONTINUATION;
                LoadTaskField(t.priority, DetailedTaskInfo::priorityFromString(
                                              v.trimmed()));
            },
        },
        {
            "scheduled",
            [](const QString &v, Task &t, Mode m) {
                SKIP_CONTINUATION;
                LoadTaskDate(t.sched, v);
            },
        },
        {
            "due",
            [](const QString &v, Task &t, Mode m) {
                SKIP_CONTINUATION;
                LoadTaskDate(t.due, v);
            },
        },
        {
            "wait",
            [](const QString &v, Task &t, Mode m) {
                SKIP_CONTINUATION;
                LoadTaskDate(t.wait, v);
            },
        },
        {
            "description",
            [](const QString &v, Task &t, Mode m) {
                // Note, we accept continuation here.
                if (const auto d = v.trimmed(); !d.isEmpty()) {
                    // The annotations always start with a timestamp. And they
                    // always follows the description.
                    if (m == Mode::AdditionalLineOfExistingRecord) {
                        const QString first_word =
                            d.section(' ', 0, 0, QString::SectionSkipEmpty)
                                .trimmed();
                        if (QDateTime::fromString(first_word, Qt::ISODate)
                                .isValid()) {
                            // We won't handle the annotations at all.
                            return;
                        }
                    }
                    LoadTaskField(t.description,
                                  t.description.get() + '\n' + d);
                }
            },
        }
    };
    return schema;
}
#undef SKIP_CONTINUATION

QStringList // NOLINT
FilteredTasksListReader::createCmdParameters(const TableStencil &stencil) const
{
    const QString columnsArg =
        QString("rc.report.minimal.columns=%1").arg(stencil.getCmdColumns());
    const QString labelsArg =
        QString("rc.report.minimal.labels=%1").arg(stencil.getCmdLabels());

    QStringList cmd = {
        columnsArg,
        labelsArg,
        "rc.report.minimal.sort=urgency-",
        "rc.print.empty.columns=yes",
        "rc.dateformat=Y-M-DTH:N:S",
        "+PENDING",
        "minimal",
    };

    if (m_filter.getIds().has_value()) {
        cmd << *m_filter.getIds();
    }

    return cmd;
}

bool FilteredTasksListReader::readUrgencySortedTaskList(
    const TaskWarriorExecutor &executor)
{
    auto response = readAndParseTable(executor);
    const LambdaVisitor visitor{
        [this](Response resp) {
            tasks = std::move(resp);
            return true;
        },
        [](const auto & /*err*/) { return false; },
    };

    return std::visit(visitor, std::move(response));
}
