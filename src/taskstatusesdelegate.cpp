#include "taskstatusesdelegate.hpp"
#include "taskhintproviderdelegate.hpp"
#include "tasksmodel.hpp"

#include <QModelIndex>
#include <QObject>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

TaskStatusesDelegate::TaskStatusesDelegate(QObject *parent)
    : TaskHintProviderDelegate(parent)
{
}

void TaskStatusesDelegate::paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const QString fullText = index.data(Qt::DisplayRole).toString();
    const QString prefix = index.data(TasksModel::TaskEmoji).toString();
    const QFontMetrics fm(opt.font);
    const int emojiWidth = fm.horizontalAdvance(prefix);

    painter->save();

    const QVariant bg = index.data(Qt::BackgroundRole);
    const QColor taskColor =
        bg.isValid() ? bg.value<QColor>() : opt.palette.base().color();
    painter->fillRect(option.rect, taskColor);

    painter->setPen(option.palette.color(QPalette::Text));
    if (opt.state & QStyle::State_Selected) {
        QRect selRect = option.rect;
        selRect.setLeft(option.rect.left() + emojiWidth);

        const auto highlightColor = getHighlightColor(option);
        const int transitionWidth = 15;
        if (selRect.width() > transitionWidth) {
            // Рисуем переход
            QLinearGradient grad(selRect.left(), 0,
                                 selRect.left() + transitionWidth, 0);
            grad.setColorAt(0.0, taskColor);
            grad.setColorAt(1.0, highlightColor);
            painter->fillRect(QRect(selRect.left(), selRect.top(),
                                    transitionWidth, selRect.height()),
                              grad);

            // Оставшуюся часть под текстом заливаем плотным цветом
            QRect solidRect = selRect.adjusted(transitionWidth, 0, 0, 0);
            painter->fillRect(solidRect, highlightColor);
        } else {
            painter->fillRect(selRect, highlightColor);
        }
        painter->setPen(opt.palette.color(QPalette::HighlightedText));
    }

    // Drawing text
    painter->setFont(opt.font);
    painter->drawText(option.rect, opt.displayAlignment, fullText);

    painter->restore();
}
