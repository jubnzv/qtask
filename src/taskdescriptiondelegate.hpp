#ifndef TASKDESCRIPTIONDELEGATE_HPP
#define TASKDESCRIPTIONDELEGATE_HPP

#include <QPainter>
#include <QString>
#include <QStyledItemDelegate>

class TaskDescriptionDelegate : public QStyledItemDelegate {
    Q_OBJECT

  public:
    TaskDescriptionDelegate(QObject *parent = nullptr);

    QString anchorAt(const QString &markdown, const QPoint &point) const;

  protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};

#endif // TASKDESCRIPTIONDELEGATE_HPP
