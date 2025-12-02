#ifndef TASKDESCRIPTIONDELEGATE_HPP
#define TASKDESCRIPTIONDELEGATE_HPP

#include <QObject>
#include <QPainter>
#include <QString>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

#include "taskhintproviderdelegate.hpp"

class TaskDescriptionDelegate : public TaskHintProviderDelegate {
    Q_OBJECT

  public:
    explicit TaskDescriptionDelegate(QObject *parent = nullptr);

    QString anchorAt(const QString &markdown, const QPoint &point) const;

  protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    [[nodiscard]]
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};

#endif // TASKDESCRIPTIONDELEGATE_HPP
