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

/// @brief This delegate provides detailed tooltip on row.
///
/// @note We implement custom tooltip handling (using QStyledItemDelegate)
/// instead of relying on the model's Qt::ToolTipRole.
///
/// Rationale: When switching between the "loading" tooltip state and the "full
/// details" state, using the model handler would require the user to move the
/// mouse to trigger the new content. The delegate allows us to dynamically
/// **update the tooltip content in place** without requiring any mouse
/// movement, resulting in a much smoother and better user experience.
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
