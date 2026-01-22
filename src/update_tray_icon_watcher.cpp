#include "update_tray_icon_watcher.hpp"
#include "tabular_stencil_base.hpp"
#include "task.hpp"
#include "task_date_time.hpp"

#include <QString>
#include <QStringList>

#include <algorithm>
#include <chrono>

namespace
{
using namespace std::chrono_literals;

template <typename T>
constexpr T constexpr_min(const T a, const T b)
{
    return std::min(a, b);
}

template <typename T, typename... Args>
constexpr T constexpr_min(const T a, Args... args)
{
    return constexpr_min(a, constexpr_min(args...));
}

/// @brief Combined refresh interval of how often we will check status below.
constexpr auto kRefreshInterval = []() {
    static constexpr std::chrono::milliseconds kDefaultRefresh = 5min; // NOLINT
    return constexpr_min(
        kDefaultRefresh,
        TaskDateTime<ETaskDateTimeRole::Due>::warning_interval(),
        TaskDateTime<ETaskDateTimeRole::Sched>::warning_interval(),
        TaskDateTime<ETaskDateTimeRole::Wait>::warning_interval());
}();

// Example we want to achieve:
// task +PENDING "(+ACTIVE or (due < now+%1) or (scheduled < now+%2) or (wait <
// now+%3))" rc.report.status.columns=id,due,scheduled,wait,start
// rc.report.status.labels=',|,|,|,|' rc.print.empty.columns=yes
// rc.dateformat=Y-M-DTH:N:S '(due < now+2d)' '(scheduled < now+1h)' status

#define SKIP_CONTINUATION                                      \
    if (m != ColumnDescriptor::LineMode::FirstLineOfNewRecord) \
    return

class UpcomingTasksStencil : public TabularStencilBase<DetailedTaskInfo> {
  public:
    using TabularStencilBase::readAndParseTable;

  protected:
    const ColumnsSchema &getSchema() const override
    {
        static const ColumnsSchema schema = {
            {
                "id",
                [](const QString &v, DetailedTaskInfo &t,
                   ColumnDescriptor::LineMode m) {
                    SKIP_CONTINUATION;
                    t.task_id = v.trimmed();
                },
            },
            {
                "uuid",
                [](const QString &v, DetailedTaskInfo &t,
                   ColumnDescriptor::LineMode m) {
                    SKIP_CONTINUATION;
                    t.task_uuid = v.trimmed();
                },
            },
            {
                "scheduled",
                [](const QString &v, DetailedTaskInfo &t,
                   ColumnDescriptor::LineMode m) {
                    SKIP_CONTINUATION;
                    LoadTaskDate(t.sched, v);
                },
            },
            {
                "due",
                [](const QString &v, DetailedTaskInfo &t,
                   ColumnDescriptor::LineMode m) {
                    SKIP_CONTINUATION;
                    LoadTaskDate(t.due, v);
                },
            },
            {
                "wait",
                [](const QString &v, DetailedTaskInfo &t,
                   ColumnDescriptor::LineMode m) {
                    SKIP_CONTINUATION;
                    LoadTaskDate(t.wait, v);
                },
            },
            {
                "start.active",
                [](const QString &v, DetailedTaskInfo &t,
                   ColumnDescriptor::LineMode m) {
                    SKIP_CONTINUATION;
                    LoadTaskField(t.active, !v.trimmed().isEmpty());
                },
            },
        };
        return schema;
    }

    QStringList createCmdParameters(const TableStencil &stencil) const override
    {
        static const QString schedLimit =
            getLimitString<ETaskDateTimeRole::Sched>();
        static const QString dueLimit =
            getLimitString<ETaskDateTimeRole::Due>();
        static const QString waitLimit =
            getLimitString<ETaskDateTimeRole::Wait>();
        static const auto filterStr =
            QString("(+ACTIVE or (due < now+%1) or (scheduled < now+%2) or "
                    "(wait < now+%3))")
                .arg(dueLimit, schedLimit, waitLimit);
        return {
            "status",
            QString("rc.report.status.columns=%1").arg(stencil.getCmdColumns()),
            QString("rc.report.status.labels=%1").arg(stencil.getCmdLabels()),
            "+PENDING",
            filterStr,
        };
    }

  private:
    template <ETaskDateTimeRole taRole>
    static QString getLimitString()
    {
        using namespace std::chrono_literals;
        auto total =
            TaskDateTime<taRole>::warning_interval() + kRefreshInterval + 1min;
        return QString("%1min").arg(
            std::chrono::duration_cast<std::chrono::minutes>(total).count());
    }
    template <typename taProperty, typename taValue>
    static void LoadTaskField(taProperty &field, taValue value)
    {
        // We don't really need here to keep fields "not modified on load", but
        // it is good for consistency.
        field = std::move(value);
        field.value.setNotModified();
    }

    template <typename taProperty>
    static void LoadTaskDate(taProperty &field, const QString &date)
    {
        auto dt = QDateTime::fromString(date.trimmed(), Qt::ISODate);
        if (dt.isValid()) {
            LoadTaskField(field, std::move(dt));
        }
    }
};

#undef SKIP_CONTINUATION

} // namespace

UpdateTrayIconWatcher::UpdateTrayIconWatcher() {}
