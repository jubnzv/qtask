#include "tasksmodel.hpp"
#include "task_emojies.hpp"

#include <utility>

#include <QAbstractTableModel>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QIcon>
#include <QList>
#include <QModelIndex>
#include <QObject>
#include <QVariant>
#include <QtCore/Qt>
#include <qlogging.h>
#include <qnamespace.h>
#include <qtmetamacros.h>

#include "task.hpp"

TasksModel::TasksModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_tasks({})
{
}

int TasksModel::rowCount(const QModelIndex & /*parent*/) const
{
    return static_cast<int>(m_tasks.size());
}

int TasksModel::columnCount(const QModelIndex & /*parent*/) const { return 3; }

QVariant TasksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    const auto &task = m_tasks.at(index.row());

    switch (role) {
    case Qt::DisplayRole: {
        switch (index.column()) {
        case 0:
            return task.task_id + taskToTimeEmojies(task);
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
    case Qt::DecorationRole:
        if (index.column() == 0) {
            return (m_tasks.at(index.row()).active)
                       ? QIcon(":/icons/active.svg")
                       : QVariant{};
        }
        break;
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
                         int role)
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

QVariant TasksModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (orientation == Qt::Horizontal) {
            switch (section) {
            case 0:
                return tr("id");
            case 1:
                return tr("Project");
            case 2:
                return tr("Description");
            default:
                qDebug() << "Unexpected case in TasksModel::headerData";
                break;
            }
        }
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

    for (int i = 0, sz = m_tasks.count(); i < sz; ++i) {
        // Do not read m_tasks directly, use data(), because it ensures proper
        // mapping of the index.
        const QModelIndex newIndex = createIndex(i, 0);
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

QColor TasksModel::rowColor(int row) const
{
    QColor c;

    if (row < 0 || row >= m_tasks.size()) {
        c.setRgb(0xffffff);
        return c;
    }

    switch (m_tasks.at(row).priority.get()) {
    case DetailedTaskInfo::Priority::Unset:
        c.setRgb(0xffffff);
        break;
    case DetailedTaskInfo::Priority::L:
        c.setRgb(0xf7ffe4);
        break;
    case DetailedTaskInfo::Priority::M:
        c.setRgb(0xfffae4);
        break;
    case DetailedTaskInfo::Priority::H:
        c.setRgb(0xd5acbe);
        break;
    default:
        c.setRgb(0xffffff);
        break;
    }

    return c;
}
