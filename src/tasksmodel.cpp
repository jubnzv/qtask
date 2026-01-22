#include "tasksmodel.hpp"
#include "block_guard.hpp"
#include "configmanager.hpp"

#include <array>
#include <chrono>
#include <utility>

#include <QAbstractTableModel>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QIcon>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QPalette>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <QtCore/Qt>
#include <qlogging.h>
#include <qnamespace.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "task.hpp"
#include "task_emojies.hpp"
#include "task_ids_providers.hpp"

namespace
{
using namespace std::chrono_literals;
constexpr auto kRefresheEmojiPeriod = 5s; // NOLINT

const std::array<QString, 3> kColumnsHeaders = {
    QObject::tr("Status / Id"),
    QObject::tr("Project"),
    QObject::tr("Description"),
};

QColor getColorForPriority(DetailedTaskInfo::Priority priority)
{
    QColor base = qApp->palette().color(QPalette::Base);
    QColor tint;

    switch (priority) {
    case DetailedTaskInfo::Priority::L:
        tint = Qt::green;
        break;
    case DetailedTaskInfo::Priority::M:
        tint = Qt::yellow;
        break;
    case DetailedTaskInfo::Priority::H:
        tint = Qt::red;
        break;
    default:
        return base;
    }
    const bool isDark = base.value() < 128;
    const qreal factor = isDark ? 0.15 : 0.25;

    return QColor::fromRgbF(
        base.redF() * (1.0 - factor) + tint.redF() * factor,
        base.greenF() * (1.0 - factor) + tint.greenF() * factor,
        base.blueF() * (1.0 - factor) + tint.blueF() * factor);
}
} // namespace

TasksModel::TasksModel(std::shared_ptr<Taskwarrior> task_provider,
                       SelectionProvider selected_provider, QObject *parent)
    : QAbstractTableModel(parent)
    , m_tasks({})
    , m_task_provider(std::move(task_provider))
    , m_task_watcher(new TaskWatcher(this))
    , m_statuses_watcher(new TasksStatusesWatcher(
          [this]() -> const QList<DetailedTaskInfo> & {
              return std::as_const(m_tasks);
          },
          this, kRefresheEmojiPeriod))
    , m_selected_provider(std::move(selected_provider))
{
    m_urgency_signaler.setSingleShot(true);
    m_urgency_signaler.setInterval(150);

    connect(m_statuses_watcher, &TasksStatusesWatcher::statusesWereChanged,
            this, &TasksModel::delayedRefreshModel);
    connect(m_task_watcher, &TaskWatcher::dataOnDiskWereChanged, this,
            &TasksModel::refreshModel);

    const auto recomputeUrgency = [this]() {
        const QDateTime now = QDateTime::currentDateTime();
        StatusEmoji::EmojiUrgency maxUrgency = StatusEmoji::EmojiUrgency::None;
        // Optimization
        if (!ConfigManager::config().get(ConfigManager::MuteNotifications)) {
            for (const auto &task : std::as_const(m_tasks)) {
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
    };

    connect(&m_urgency_signaler, &QTimer::timeout, this, recomputeUrgency);
    connect(&ConfigManager::config().notifier(), &ConfigEvents::settingsChanged,
            this, recomputeUrgency);

    // Need to trigger watcher at least once.
    m_task_watcher->checkNow();
}

int TasksModel::rowCount(const QModelIndex & /*parent*/) const
{
    return static_cast<int>(m_tasks.size());
}

int TasksModel::columnCount(const QModelIndex & /*parent*/) const
{
    return kColumnsHeaders.size();
}

QVariant TasksModel::data(const QModelIndex &index, const int role) const
{
    if (!index.isValid()) {
        return {};
    }
    const auto &task = m_tasks.at(index.row());

    switch (role) {
    case Qt::DisplayRole: {
        switch (index.column()) {
        case 0: {
            return task.task_id;
        }
        case 1:
            return task.project.get();
        case 2:
            return task.description.get();
        default:
            qDebug() << "Unexpected case in TasksModel::data";
            break;
        }
        break;
    }
    case Qt::TextAlignmentRole:
        return { Qt::AlignVCenter | Qt::AlignLeft };
    case Qt::BackgroundRole:
        return QVariant(QBrush(rowColor(index.row())));
    case TaskReadRole:
        return QVariant::fromValue(task);
    default:
        break;
    }

    return {};
}

bool TasksModel::setData(const QModelIndex &idx, const QVariant &value,
                         const int role)
{
    if (!idx.isValid()) {
        return false;
    }
    const int row = idx.row();

    switch (role) {
    case Qt::EditRole:
        qDebug()
            << "Requested edit which is not supported / implemented on row [ "
            << row << " ].";
        break;
    case TaskUpdateRole:
        if (value.canConvert<DetailedTaskInfo>()) {
            const auto guard = BlockGuard(m_task_watcher, m_statuses_watcher,
                                          &m_urgency_signaler);
            auto updatedTask = value.value<DetailedTaskInfo>();
            const QDateTime now = QDateTime::currentDateTime();

            const auto oldUrgency =
                StatusEmoji(m_tasks[row], now).getMostUrgentLevel();
            m_tasks[row] = updatedTask;
            const auto newUrgency =
                StatusEmoji(m_tasks[row], now).getMostUrgentLevel();
            if (oldUrgency == newUrgency) {
                const auto start = index(row, 0);
                const auto end = index(row, columnCount() - 1);
                emit dataChanged(start, end,
                                 { Qt::DisplayRole, Qt::BackgroundRole });
                dataUpdated();
            } else {
                // We need to go out of scope of guard
                delayedRefreshModel();
            }
            return true;
        }
        break;
    default:
        break;
    }
    // Data was NOT changed - result is false. Point.
    return false;
}

QVariant TasksModel::headerData(const int section,
                                const Qt::Orientation orientation,
                                const int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (orientation == Qt::Horizontal) {
            if (section < kColumnsHeaders.size()) {
                return kColumnsHeaders.at(section);
            }
            qDebug() << "Unexpected case in TasksModel::headerData";
        }
        break;
    default:
        break;
    }

    return {};
}

QColor TasksModel::rowColor(const int row) const
{
    if (row < 0 || row >= m_tasks.size()) {
        return getColorForPriority(DetailedTaskInfo::Priority::Unset);
    }
    return getColorForPriority(m_tasks.at(row).priority.get());
}

void TasksModel::refreshModel()
{
    const auto guard =
        BlockGuard(m_task_watcher, m_statuses_watcher, &m_urgency_signaler);
    const auto currentlySelectedTaskIds = m_selected_provider();

    // Load whole tasks list from disk
    auto tasks = m_task_provider->getUrgencySortedTasks();
    if (!tasks.has_value()) {
        return;
    }
    beginResetModel();
    m_tasks = std::move(*tasks);
    endResetModel();
    QModelIndexList indicesToSelect;

    for (qsizetype i = 0, sz = m_tasks.count(); i < sz; ++i) {
        // Do not read m_tasks directly, use data(), because it ensures proper
        // mapping of the index.
        const auto newIndex = createIndex(static_cast<int>(i), 0);
        const auto task = data(newIndex, TaskReadRole);

        if (!task.isValid() || !task.canConvert<DetailedTaskInfo>()) {
            continue;
        }
        const auto &taskUuid = kTaskUuidGetter(task.value<DetailedTaskInfo>());
        if (currentlySelectedTaskIds.contains(taskUuid)) {
            indicesToSelect.append(newIndex);
        }
    }
    if (!indicesToSelect.isEmpty()) {
        emit restoreSelected(indicesToSelect);
    }
    dataUpdated();
}

void TasksModel::refreshIfChangedOnDisk()
{
    QTimer::singleShot(50, m_task_watcher, &TaskWatcher::checkNow);
}

void TasksModel::delayedRefreshModel()
{
    // We could detect "condition changed" too fast, like 0.001s after it
    // happened. So we want to wait so taskwarrior will be ready to make newly
    // sorted list.
    QTimer::singleShot(2500, this, [this]() { refreshModel(); });
}

void TasksModel::dataUpdated()
{
    m_urgency_signaler.start();
    m_statuses_watcher->startWatchingStatusesChange();
}
