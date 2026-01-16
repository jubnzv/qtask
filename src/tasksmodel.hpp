#ifndef TASKSMODEL_HPP
#define TASKSMODEL_HPP

#include <QAbstractTableModel>
#include <QColor>
#include <QList>
#include <QModelIndex>
#include <QTimer>
#include <QVariant>
#include <QtCore/Qt>

#include "task.hpp"
#include "tasksstatuseswatcher.hpp"
#include "taskwarrior.hpp"
#include "taskwatcher.hpp"

#include <functional>
#include <memory>

class TasksModel : public QAbstractTableModel {
    Q_OBJECT
  public:
    /// @brief Provider should return IDs of the selected tasks at the moment
    /// when it was called.
    using SelectionProvider = std::function<QStringList()>;

    static constexpr auto TaskUpdateRole = Qt::UserRole + 1;
    static constexpr auto TaskReadRole = Qt::UserRole + 2;

    TasksModel(std::shared_ptr<Taskwarrior> task_provider,
               SelectionProvider selected_provider, QObject *parent = nullptr);

    [[nodiscard]]
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    [[nodiscard]]
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    [[nodiscard]]
    QVariant data(const QModelIndex &index,
                  int role = Qt::DisplayRole) const override;
    [[nodiscard]]
    bool setData(const QModelIndex &, const QVariant &value,
                 int role = Qt::EditRole) override;

    [[nodiscard]]
    QVariant headerData(int section, Qt::Orientation,
                        int role = Qt::DisplayRole) const override;

    [[nodiscard]]
    QVariant getTask(const QModelIndex &) const;

    [[nodiscard]] QColor rowColor(int row) const;

  signals:
    /// @brief View can listen this signal if it wants to restore selection
    /// after model reset.
    /// @note Indexes will be different than it was, as tasks list could be
    /// reordered/resized.
    void restoreSelected(const QModelIndexList &);

  public slots:
    /// @brief Queries taskwatcher for the fresh/current sorted list of the
    /// tasks.
    void refreshModel();

    /// @brief This is lazy refresh, if it was no changes on disk, it will do
    /// nothing. Which means it will NOT react for time-going changes.
    void refreshIfChangedOnDisk();

  private:
    QList<DetailedTaskInfo> m_tasks;

    std::shared_ptr<Taskwarrior> m_task_provider;
    TaskWatcher *m_task_watcher;
    TasksStatusesWatcher *m_statuses_watcher;
    SelectionProvider m_selected_provider;
};

#endif // TASKSMODEL_HPP
