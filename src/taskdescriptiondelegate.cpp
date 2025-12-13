#include "taskdescriptiondelegate.hpp"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QObject>
#include <QSize>
#include <QString>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QtAssert>
#include <QtVersionChecks>

#include "taskhintproviderdelegate.hpp"
#include "tasksmodel.hpp"

// FIXME: this is sort of broken, for example it will not show \\ as markdown
// but without markdown it will draw multiline string outside limits.
namespace
{

void initDocument(QTextDocument &document, const QString &whole_text)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    document.setMarkdown(whole_text);
#else
    document.setPlainText(whole_text);
#endif // QT_VERSION_CHECK
}

/// @returns true if elide is needed.
bool initDocument(QTextDocument &document, const QStyleOptionViewItem &option,
                  const QString &whole_text)
{
    initDocument(document, whole_text);
    document.setTextWidth(option.rect.width());

    const QFontMetrics fm(option.font);
    const bool elide_needed = document.size().height() / fm.height() > 1.5f;

    document.setPageSize(option.rect.size());

    return elide_needed;
}

} // namespace
TaskDescriptionDelegate::TaskDescriptionDelegate(QObject *parent)
    : TaskHintProviderDelegate(parent)
    , document(new QTextDocument(this))
{
}

QString TaskDescriptionDelegate::anchorAt(const QString &markdown,
                                          const QPoint &point) const
{
    initDocument(*document, markdown);
    auto textLayout = document->documentLayout();
    Q_ASSERT(textLayout != nullptr);
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
    const auto value = index.data(Qt::DisplayRole);
    if (value.isValid() && !value.isNull()) {
        const bool elideNeeded =
            initDocument(*document, option, value.toString());
        painter->translate(option.rect.topLeft());
        document->drawContents(
            painter, { 0, 0, option.rect.width(), option.rect.height() });

        // Need to add ...
        if (elideNeeded) {
            const QString ellipsis = QStringLiteral("…");
            const QFontMetrics fm(option.font);
            const int ellipsisWidth = fm.horizontalAdvance(ellipsis);
            const QPoint ellipsisPos(option.rect.width() - ellipsisWidth,
                                     fm.ascent());
            painter->setPen(
                option.palette.color((option.state & QStyle::State_Selected)
                                         ? QPalette::HighlightedText
                                         : QPalette::Text));

            painter->drawText(ellipsisPos, ellipsis);
        }
    }

    painter->restore();
}

QSize TaskDescriptionDelegate::sizeHint(const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    QStyleOptionViewItem options = option;
    initStyleOption(&options, index);
    initDocument(*document, options, options.text);
    return { document->idealWidth(), options.rect.height() };
}
