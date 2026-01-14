#include "tasksmodel.hpp"
#include "task_emojies.hpp"

#include <array>
#include <chrono>
#include <utility>

#include <QAbstractTableModel>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QIcon>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QVariant>
#include <QtCore/Qt>
#include <qlogging.h>
#include <qnamespace.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "task.hpp"

namespace
{
using namespace std::chrono_literals;
constexpr auto kRefresheEmojiPeriod = 30s; // NOLINT

const std::array<QString, 3> kColumnsHeaders = {
    QObject::tr("Id / Status"),
    QObject::tr("Project"),
    QObject::tr("Description"),
};

QColor getColorForPriority(DetailedTaskInfo::Priority priority)
{
    switch (priority) {
    case DetailedTaskInfo::Priority::L:
        return { 0xffe8f5e9 };
    case DetailedTaskInfo::Priority::M:
        return { 0xfffff9c4 };
    case DetailedTaskInfo::Priority::H:
        return { 0xffffcdd2 };
    case DetailedTaskInfo::Priority::Unset:
        break;
    }
    return { 0xffffffff };
}
} // namespace

TasksModel::TasksModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_tasks({})
    , m_refresh_emoji_timer(new QTimer(this))
{
    connect(m_refresh_emoji_timer, &QTimer::timeout, this, [this]() {
        if (rowCount() > 0) {
            emit dataChanged(index(0, 0), index(rowCount() - 1, 0),
                             { Qt::DisplayRole, Qt::ToolTipRole });
        }
    });
    m_refresh_emoji_timer->start(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            kRefresheEmojiPeriod)
            .count());
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
            const StatusEmoji status(task);
            const QString emojis = status.combinedEmoji();
            if (emojis.isEmpty()) {
                return task.task_id;
            }
            return " " + task.task_id + " " + emojis + " ";
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
            auto updatedTask = value.value<DetailedTaskInfo>();
            m_tasks[row] = updatedTask;
            const auto start = index(row, 0);
            const auto end = index(row, columnCount() - 1);
            emit dataChanged(start, end,
                             { Qt::DisplayRole, Qt::BackgroundRole });
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

void TasksModel::setTasks(QList<DetailedTaskInfo> tasks,
                          const QStringList &currentlySelectedTaskIds)
{
    beginResetModel();
    m_tasks = std::move(tasks);
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
        // FIXME: need stable UUID here instead task_id.
        // TODO:  switch to `task rc.json.array=on export` instead console.
        const auto taskUuid = task.value<DetailedTaskInfo>().task_id;
        if (currentlySelectedTaskIds.contains(taskUuid)) {
            indicesToSelect.append(newIndex);
        }
    }
    if (!indicesToSelect.isEmpty()) {
        emit selectIndices(indicesToSelect);
    }
}

QVariant TasksModel::getTask(const QModelIndex &index) const
{
    if (index.isValid() && index.row() < m_tasks.size()) {
        return QVariant::fromValue(m_tasks.at(index.row()));
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
