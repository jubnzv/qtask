#ifndef TASKSMODEL_HPP
#define TASKSMODEL_HPP

#include <QAbstractTableModel>
#include <QColor>
#include <QList>
#include <QModelIndex>
#include <QVariant>

#include "task.hpp"

class TasksModel : public QAbstractTableModel {
    Q_OBJECT
  public:
    TasksModel(QObject *parent = nullptr);
    ~TasksModel() = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &, const QVariant &,
                 int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation,
                        int role = Qt::DisplayRole) const override;

    void setTasks(const QList<Task> &);
    void setTasks(QList<Task> &&);

    QColor getTaskColor(const Task &) const;

  private:
    QList<Task> m_tasks;
};

#endif // TASKSMODEL_HPP
