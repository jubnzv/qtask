#ifndef TASKSMODEL_HPP
#define TASKSMODEL_HPP

#include <QAbstractTableModel>
#include <QColor>
#include <QList>
#include <QModelIndex>
#include <QVariant>
#include <QtCore/Qt>

#include "task.hpp"

class TasksModel : public QAbstractTableModel {
    Q_OBJECT
  public:
    static constexpr auto TaskUpdateRole = Qt::UserRole + 1;
    static constexpr auto TaskReadRole = Qt::UserRole + 2;

    explicit TasksModel(QObject *parent = nullptr);

    [[nodiscard]]
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    [[nodiscard]]
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    [[nodiscard]]
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex &, const QVariant &value,
                 int role = Qt::EditRole) override;

    [[nodiscard]]
    QVariant headerData(int section, Qt::Orientation,
                        int role = Qt::DisplayRole) const override;

    void setTasks(QList<DetailedTaskInfo>,
                  const QStringList &currentlySelectedTaskIds);

    [[nodiscard]]
    QVariant getTask(const QModelIndex &) const;

    QColor rowColor(int row) const;
  signals:
    // View can listen this signal if it wants to restore selection after model
    // reset.
    void selectIndices(const QModelIndexList &);

  private:
    QList<DetailedTaskInfo> m_tasks;
};

#endif // TASKSMODEL_HPP
