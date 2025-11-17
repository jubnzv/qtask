#include "recurringtasksmodel.hpp"

#include <QBrush>
#include <QDebug>
#include <QIcon>
#include <QList>
#include <QVariant>

#include <utility>

RecurringTasksModel::RecurringTasksModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_tasks({})
{
}

int RecurringTasksModel::rowCount(const QModelIndex & /*parent*/) const
{
    return m_tasks.size();
}

int RecurringTasksModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 4;
}

QVariant RecurringTasksModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        RecurringTask task = m_tasks.at(index.row());
        switch (index.column()) {
        case 0:
            return task.uuid;
        case 1:
            return task.project;
        case 2:
            return task.period;
        case 3:
            return task.description;
        default:
            return false;
        }
    }

    return {};
}

bool RecurringTasksModel::setData(const QModelIndex &idx, const QVariant &value,
                                  int role)
{
    if (!idx.isValid()) {
        return false;
    }

    if (role == Qt::EditRole) {
        int row = idx.row();
        qDebug() << "Requested edit " << row;
    }

    return true;
}

QVariant RecurringTasksModel::headerData(int section,
                                         Qt::Orientation orientation,
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
            return tr("Period");
        case 3:
            return tr("Description");
        }
    }

    return {};
}

void RecurringTasksModel::setTasks(QList<RecurringTask> tasks)
{
    beginResetModel();
    m_tasks = std::move(tasks);
    endResetModel();
}
