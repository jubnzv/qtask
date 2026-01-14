#include "taskstatusesdelegate.hpp"
#include "task.hpp"
#include "task_emojies.hpp"
#include "taskhintproviderdelegate.hpp"
#include "tasksmodel.hpp"

#include <QLinearGradient>
#include <QModelIndex>
#include <QObject>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <qnamespace.h>

TaskStatusesDelegate::TaskStatusesDelegate(QObject *parent)
    : TaskHintProviderDelegate(parent)
{
}

void TaskStatusesDelegate::paint(QPainter *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    const auto task_var = index.data(TasksModel::TaskReadRole);
    if (!task_var.canConvert<DetailedTaskInfo>()) {
        return;
    }
    const auto task = task_var.value<DetailedTaskInfo>();

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const auto idText = index.data(Qt::DisplayRole).toString();
    const auto prefix = StatusEmoji(task).alignedCombinedEmoji();
    const QFontMetrics fm(opt.font);
    const int emojiWidth = fm.horizontalAdvance(prefix);

    painter->save();

    const auto bg = index.data(Qt::BackgroundRole);
    const auto taskColor =
        bg.isValid() ? bg.value<QColor>() : opt.palette.base().color();
    painter->fillRect(option.rect, taskColor);

    painter->setPen(option.palette.color(QPalette::Text));
    if (opt.state & QStyle::State_Selected) {
        QRect selRect = option.rect;
        selRect.setLeft(option.rect.left() + emojiWidth);

        const auto highlightColor = getHighlightColor(option);
        const int transitionWidth = 15;
        if (selRect.width() > transitionWidth) {
            QLinearGradient grad(selRect.left(), 0,
                                 selRect.left() + transitionWidth, 0);
            grad.setColorAt(0.0, taskColor);
            grad.setColorAt(1.0, highlightColor);
            painter->fillRect(QRect(selRect.left(), selRect.top(),
                                    transitionWidth, selRect.height()),
                              grad);
            const auto solidRect = selRect.adjusted(transitionWidth, 0, 0, 0);
            painter->fillRect(solidRect, highlightColor);
        } else {
            painter->fillRect(selRect, highlightColor);
        }
        painter->setPen(opt.palette.color(QPalette::HighlightedText));
    }

    // Drawing text
    painter->setFont(opt.font);
    painter->drawText(option.rect, opt.displayAlignment, prefix + " " + idText);

    painter->restore();
}
