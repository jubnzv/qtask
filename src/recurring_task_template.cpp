#include "recurring_task_template.hpp"
#include "tabular_stencil_base.hpp"
#include "taskwarriorexecutor.hpp"

#include <QList>
#include <QString>

#include <optional>

namespace
{
#define SKIP_CONTINUATION                                      \
    if (m != ColumnDescriptor::LineMode::FirstLineOfNewRecord) \
    return

class RecurringTasksReader : public TabularStencilBase<RecurringTaskTemplate> {
  public:
    using TabularStencilBase::readAndParseTable;

  protected:
    const ColumnsSchema &getSchema() const override
    {
        static const ColumnsSchema schema = {
            {
                "id",
                [](const QString &v, RecurringTaskTemplate &t,
                   ColumnDescriptor::LineMode m) {
                    SKIP_CONTINUATION;
                    t.uuid = v.trimmed();
                },
            },
            {
                "recur",
                [](const QString &v, RecurringTaskTemplate &t,
                   ColumnDescriptor::LineMode m) {
                    SKIP_CONTINUATION;
                    t.period = v.trimmed();
                },
            },
            {
                "project",
                [](const QString &v, RecurringTaskTemplate &t,
                   ColumnDescriptor::LineMode m) {
                    SKIP_CONTINUATION;
                    t.project = v.trimmed();
                },
            },
            {
                "description",
                [](const QString &v, RecurringTaskTemplate &t,
                   ColumnDescriptor::LineMode) {
                    if (t.description.isEmpty()) {
                        t.description = v.trimmed();
                    } else {
                        t.description += "\n" + v.trimmed();
                    }
                },
            }
        };
        return schema;
    }

    QStringList createCmdParameters(const TableStencil &stencil) const override
    {
        return { "recurring_full",
                 QString("rc.report.recurring_full.columns=%1")
                     .arg(stencil.getCmdColumns()),
                 QString("rc.report.recurring_full.labels=%1")
                     .arg(stencil.getCmdLabels()),
                 "status:Recurring", "rc.print.empty.columns=yes" };
    }
};
#undef SKIP_CONTINUATION

} // namespace

std::optional<QList<RecurringTaskTemplate>>
RecurringTaskTemplate::readAll(const TaskWarriorExecutor &executor)
{
    auto response = RecurringTasksReader().readAndParseTable(executor);

    if (auto *tasks = std::get_if<RecurringTasksReader::Response>(&response)) {
        return *tasks;
    }
    return std::nullopt;
}
