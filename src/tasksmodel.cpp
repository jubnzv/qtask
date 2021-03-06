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
        if (index.column() == 0) {
            Task task = m_tasks.at(index.row());
            return (task.active) ? QIcon(":/icons/active.svg") : QVariant();
        }
    }

    else if (role == Qt::BackgroundRole) {
        return QVariant(QBrush(rowColor(index.row())));
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

QVariant TasksModel::getTask(const QModelIndex &index) const
{
    if (index.isValid() && index.row() < m_tasks.size())
        return QVariant::fromValue(m_tasks.at(index.row()));
    return {};
}

QColor TasksModel::rowColor(int row) const
{
    QColor c;

    if (row < 0 || row >= m_tasks.size()) {
        c.setRgb(0xffffff);
        return c;
    }

    switch (m_tasks.at(row).priority) {
    case Task::Priority::Unset:
        c.setRgb(0xffffff);
        break;
    case Task::Priority::L:
        c.setRgb(0xf7ffe4);
        break;
    case Task::Priority::M:
        c.setRgb(0xfffae4);
        break;
    case Task::Priority::H:
        c.setRgb(0xd5acbe);
        break;
    default:
        c.setRgb(0xffffff);
        break;
    }

    return c;
}
