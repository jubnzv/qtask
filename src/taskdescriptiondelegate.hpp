#ifndef TASKDESCRIPTIONDELEGATE_HPP
#define TASKDESCRIPTIONDELEGATE_HPP

#include <QPainter>
#include <QStyledItemDelegate>

class TaskDescriptionDelegate : public QStyledItemDelegate {
    Q_OBJECT

  public:
    TaskDescriptionDelegate(QObject *parent = nullptr);

  protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

#endif // TASKDESCRIPTIONDELEGATE_HPP
