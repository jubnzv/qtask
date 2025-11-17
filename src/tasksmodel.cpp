#include "tasksmodel.hpp"

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
#include <qlogging.h>
#include <qnamespace.h>

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
    if (role == Qt::DisplayRole) {
        const Task task = m_tasks.at(index.row());
        switch (index.column()) {
        case 0:
            return task.uuid;
        case 1:
            return task.project;
        case 2:
            return task.description;
        default:
            qDebug() << "Unexpected case in TasksModel::data";
            break;
        }
    } else if (role == Qt::DecorationRole) {
        if (index.column() == 0) {
            return (m_tasks.at(index.row()).active)
                       ? QIcon(":/icons/active.svg")
                       : QVariant{};
        }
    } else if (role == Qt::BackgroundRole) {
        return QVariant(QBrush(rowColor(index.row())));
    }

    return {};
}

bool TasksModel::setData(const QModelIndex &idx, const QVariant & /*value*/,
                         int role)
{
    if (!idx.isValid()) {
        return false;
    }

    if (role == Qt::EditRole) {
        const int row = idx.row();
        qDebug() << "Requested edit " << row;
    }

    return true;
}

QVariant TasksModel::headerData(int section, Qt::Orientation orientation,
                                int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

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

    return {};
}

void TasksModel::setTasks(QList<Task> tasks)
{
    beginResetModel();
    m_tasks = std::move(tasks);
    endResetModel();
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
