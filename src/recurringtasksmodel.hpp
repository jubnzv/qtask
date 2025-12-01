#ifndef RECURRINGTASKSMODEL_HPP
#define RECURRINGTASKSMODEL_HPP

#include <QAbstractTableModel>
#include <QList>
#include <QModelIndex>
#include <QVariant>

#include "task.hpp"

class RecurringTasksModel : public QAbstractTableModel {
    Q_OBJECT
  public:
    RecurringTasksModel(QObject *parent = nullptr);
    ~RecurringTasksModel() = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &, const QVariant &,
                 int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation,
                        int role = Qt::DisplayRole) const override;

    void setTasks(QList<RecurringTaskTemplate> tasks);

  private:
    QList<RecurringTaskTemplate> m_tasks;
};

#endif // RECURRINGTASKSMODEL_HPP
