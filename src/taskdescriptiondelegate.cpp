#include "taskdescriptiondelegate.hpp"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QTextDocument>

#include "tasksmodel.hpp"

// FIXME: this is sort of broken, for example it will not show \\ as markdown
// but without markdown it will draw multiline string outside limits.

TaskDescriptionDelegate::TaskDescriptionDelegate(QObject *parent)
    : TaskHintProviderDelegate(parent)
{
}

QString TaskDescriptionDelegate::anchorAt(const QString &markdown,
                                          const QPoint &point) const
{
    QTextDocument doc;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    doc.setMarkdown(markdown);
#else
    doc.setPlainText(markdown);
#endif // QT_VERSION_CHECK

    auto textLayout = doc.documentLayout();
    Q_ASSERT(textLayout != 0);

    return textLayout->anchorAt(point);
}

void TaskDescriptionDelegate::paint(QPainter *painter,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
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

    QVariant value = index.data(Qt::DisplayRole);
    if (value.isValid() && !value.isNull()) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        document.setMarkdown(value.toString());
#else
        document.setPlainText(value.toString());
#endif // QT_VERSION_CHECK
        painter->translate(option.rect.topLeft());
        document.drawContents(painter);
    }

    painter->restore();
}

QSize TaskDescriptionDelegate::sizeHint(const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);

    QTextDocument doc;
    doc.setHtml(options.text);
    doc.setTextWidth(options.rect.width());

    return QSize(doc.idealWidth(), doc.size().height());
}
