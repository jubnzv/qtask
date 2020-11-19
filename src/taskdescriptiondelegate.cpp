#include "taskdescriptiondelegate.hpp"

#include <QApplication>
#include <QStyledItemDelegate>
#include <QTextDocument>

#include "tasksmodel.hpp"

TaskDescriptionDelegate::TaskDescriptionDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void TaskDescriptionDelegate::paint(QPainter *painter,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    QColor fg_color = QApplication::palette().text().color();
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    } else {
        auto *model = qobject_cast<const TasksModel *>(index.model());
        Q_ASSERT(model);
        painter->fillRect(option.rect, model->rowColor(index.row()));
    }

    painter->save();

    QTextDocument document;
    document.setTextWidth(option.rect.width());
    document.setPageSize(option.rect.size());

    QPalette pal;
    pal.setColor(QPalette::Text, fg_color);

    QVariant value = index.data(Qt::DisplayRole);

    if (value.isValid() && !value.isNull()) {
        document.setMarkdown(value.toString());
        painter->translate(option.rect.topLeft());
        document.drawContents(painter);
    }

    painter->restore();
}
