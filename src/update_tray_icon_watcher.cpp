#include "update_tray_icon_watcher.hpp"
#include "configmanager.hpp"
#include "lambda_visitors.hpp"
#include "pereodic_async_executor.hpp"
#include "tabular_stencil_base.hpp"
#include "task.hpp"
#include "task_date_time.hpp"
#include "task_emojies.hpp"
#include "tasksstatuseswatcher.hpp"
#include "taskwarriorexecutor.hpp"

#include <QDateTime>
#include <QString>
#include <QStringList>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <utility>

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
constexpr auto kRefreshFromDbInterval = []() {
    static constexpr std::chrono::milliseconds kDefaultRefresh = 5min; // NOLINT
    return constexpr_min(
        kDefaultRefresh,
        TaskDateTime<ETaskDateTimeRole::Due>::warning_interval(),
        TaskDateTime<ETaskDateTimeRole::Sched>::warning_interval(),
        TaskDateTime<ETaskDateTimeRole::Wait>::warning_interval());
}();

constexpr auto kRefreshIconInterval = 7s;
static_assert(kRefreshIconInterval < kRefreshFromDbInterval);

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
            QString("'(+ACTIVE or (due < now+%1) or (scheduled < now+%2) or "
                    "(wait < now+%3))'")
                .arg(dueLimit, schedLimit, waitLimit);
        return {
            "status",
            QString("rc.report.status.columns=%1").arg(stencil.getCmdColumns()),
            QString("rc.report.status.labels='%1'").arg(stencil.getCmdLabels()),
            "+PENDING",
            filterStr,
        };
    }

  private:
    template <ETaskDateTimeRole taRole>
    static QString getLimitString()
    {
        using namespace std::chrono_literals;
        auto total = TaskDateTime<taRole>::warning_interval() +
                     kRefreshFromDbInterval + 1min;
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

UpdateTrayIconWatcher::UpdateTrayIconWatcher(QObject *parent)
    : QObject(parent)
    , m_statuses_watcher(new TasksStatusesWatcher(
          [this]() -> const QList<DetailedTaskInfo> & { return m_hot_tasks; },
          this, kRefreshIconInterval))
{
    auto threadBody =
        [](QString pathToBinary) -> UpcomingTasksStencil::Response {
        try {
            // This is non-GUI thread.
            const TaskWarriorExecutor executor(
                std::move(pathToBinary),
                TaskWarriorExecutor::TSkipBinaryValidation{});
            auto responseOrError =
                UpcomingTasksStencil().readAndParseTable(executor);
            const LambdaVisitor visitor = {
                [](UpcomingTasksStencil::Response resp) { return resp; },
                []([[maybe_unused]] auto err) {
                    return UpcomingTasksStencil::Response{};
                },
            };
            return std::visit(visitor, std::move(responseOrError));

        } catch (...) { // NOLINT
        }
        return {};
    };
    auto paramsForThread = []() {
        QString path = ConfigManager::config().get(ConfigManager::TaskBin);
        return std::make_tuple(std::move(path));
    };
    auto receiverFromThread = [this](UpcomingTasksStencil::Response hotTasks) {
        // This is GUI thread.
        // hotTasks may require icon change in 1 minute later. However, next
        // update will be in 5+ minutes later. So "between updates" must be
        // processed too.
        m_hot_tasks = std::move(hotTasks);
        recomputeUrgency();
        m_statuses_watcher->startWatchingStatusesChange();
    };

    // Setup async DB poll.
    static constexpr int kCheckPeriod =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            kRefreshFromDbInterval)
            .count();
    m_pereodic_worker = createPereodicAsynExec(
        kCheckPeriod, std::move(threadBody), std::move(paramsForThread),
        std::move(receiverFromThread));

    // Setup icon updater based on last poll.
    connect(&ConfigManager::config().notifier(), &ConfigEvents::settingsChanged,
            this, &UpdateTrayIconWatcher::recomputeUrgency);
    connect(m_statuses_watcher, &TasksStatusesWatcher::statusesWereChanged,
            this, [this]() {
                recomputeUrgency();
                m_statuses_watcher->startWatchingStatusesChange();
            });
}

void UpdateTrayIconWatcher::checkNow()
{
    assert(m_pereodic_worker);
    m_pereodic_worker->execNow();
}

void UpdateTrayIconWatcher::recomputeUrgency()
{
    const QDateTime now = QDateTime::currentDateTime();
    StatusEmoji::EmojiUrgency maxUrgency = StatusEmoji::EmojiUrgency::None;
    // Optimization
    if (!ConfigManager::config().get(ConfigManager::MuteNotifications)) {
        for (const auto &task : std::as_const(m_hot_tasks)) {
            const StatusEmoji helper(task, now);
            const auto current = helper.getMostUrgentLevel();
            if (current > maxUrgency) {
                maxUrgency = current;
            }
            if (maxUrgency == StatusEmoji::EmojiUrgency::Overdue) {
                break;
            }
        }
    }
    emit globalUrgencyChanged(maxUrgency);
}
