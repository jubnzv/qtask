#pragma once

#include <QAbstractItemView>
#include <QHelpEvent>
#include <QModelIndex>
#include <QObject>
#include <QPointer>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QWidget>

#include <atomic>

#include "async_task_loader.hpp"

class TaskHintProviderDelegate : public QStyledItemDelegate {
    Q_OBJECT
  public:
    struct TaskLoaderParameters {
        QPersistentModelIndex index;
        QPointer<QAbstractItemView> view;
        QPoint cursor_pos;
    };

    explicit TaskHintProviderDelegate(QObject *parent = nullptr);

    bool helpEvent(QHelpEvent *event, QAbstractItemView *view,
                   const QStyleOptionViewItem &option,
                   const QModelIndex &index) override;

  protected:
    [[nodiscard]]
    virtual const QString &getToolTipFooter() const;

  private:
    AsyncTaskLoader *m_task_loader;
    // This is used to cancel timer.
    std::atomic<AsyncTaskLoader::RequestId> m_pendingLoadingRequestId{
        AsyncTaskLoader::kInvalidRequestId
    };
};
Q_DECLARE_METATYPE(TaskHintProviderDelegate::TaskLoaderParameters)
