#include "tasksmodel.hpp"

#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QIcon>
#include <QList>
#include <QVariant>

TasksModel::TasksModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_tasks({})
{
}

int TasksModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_tasks.size();
}

int TasksModel::columnCount(const QModelIndex & /*parent*/) const { return 3; }

QVariant TasksModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        Task task = m_tasks.at(index.row());
        switch (index.column()) {
        case 0:
            return task.uuid;
        case 1:
            return task.project;
        case 2:
            return task.description;
        default:
            return false;
        }
    }

    else if (role == Qt::DecorationRole) {
        if (index.column() == 2) {
            Task task = m_tasks.at(index.row());
            return (task.active) ? QIcon(":/img/active.svg") : QVariant();
        }
    }

    else if (role == Qt::BackgroundRole) {
        const Task task = m_tasks.at(index.row());
        return QVariant(QBrush(getTaskColor(task)));
    }

    return QVariant();
}

bool TasksModel::setData(const QModelIndex &idx, const QVariant &value,
                         int role)
{
    if (!idx.isValid())
        return false;

    if (role == Qt::EditRole) {
        int row = idx.row();
        qDebug() << "Requested edit " << row;
    }

    return true;
}

QVariant TasksModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (role != Qt::DisplayRole)
        return {};

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return tr("id");
        case 1:
            return tr("Project");
        case 2:
            return tr("Description");
        }
    }

    return {};
}

void TasksModel::setTasks(const QList<Task> &tasks)
{
    beginResetModel();
    m_tasks = tasks;
    endResetModel();
}

void TasksModel::setTasks(QList<Task> &&tasks)
{
    beginResetModel();
    m_tasks = tasks;
    endResetModel();
}

QColor TasksModel::getTaskColor(const Task &task) const
{
    QColor c;

    switch (task.priority) {
    case Task::Priority::Unset:
        c.setNamedColor("#ffffff");
        break;
    case Task::Priority::L:
        c.setNamedColor("#f7ffe4");
        break;
    case Task::Priority::M:
        c.setNamedColor("#fffae4");
        break;
    case Task::Priority::H:
        c.setNamedColor("#d5acbe");
        break;
    default:
        c.setNamedColor("#ffffff");
        break;
    }

    return c;
}
