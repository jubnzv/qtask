#pragma once

#include <QModelIndex>
#include <QObject>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QWidget>

#include "taskhintproviderdelegate.hpp"

/// @brief This delegate excludes emoji from selection in cell. Selection is
/// painted AFTER emoji.
class TaskStatusesDelegate : public TaskHintProviderDelegate {
    Q_OBJECT
  public:
    explicit TaskStatusesDelegate(QObject *parent = nullptr);

  protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};
