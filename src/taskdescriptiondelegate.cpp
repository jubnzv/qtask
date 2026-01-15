#include "taskdescriptiondelegate.hpp"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QLinearGradient>
#include <QObject>
#include <QSize>
#include <QString>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QtAssert>
#include <QtMinMax>

#include "qtutil.hpp"
#include "taskhintproviderdelegate.hpp"
#include "tasksmodel.hpp"

namespace
{
/// @returns true if elide is needed.
bool initDocument(QTextDocument &document, const QStyleOptionViewItem &option,
                  const QString &whole_text)
{
    setContentOfTextDocument(document, whole_text);
    document.setTextWidth(option.rect.width());

    // Note, here we assume 1 line rows. If ever it could be more than 1 line
    // per 1 row, it should be revised.
    const QFontMetrics fm(option.font);
    const bool elide_needed =
        document.size().height() / qMax<qreal>(1.0, fm.height()) > 1.5f;

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
    setContentOfTextDocument(*document, markdown);
    auto textLayout = document->documentLayout();
    Q_ASSERT(textLayout != nullptr);
    return textLayout->anchorAt(point);
}

void TaskDescriptionDelegate::paint(QPainter *painter,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    auto *model = qobject_cast<const TasksModel *>(index.model());
    const auto rowColor = model->rowColor(index.row());
    const double fixedOffset = 20.0;

    if (option.state & QStyle::State_Selected) {
        const auto highlight = getHighlightColor(option);
        const double length = option.rect.width();
        const double stopPoint =
            (length > 0) ? std::min(fixedOffset / length, 0.4) : 0.0;

        QLinearGradient gradient(option.rect.topLeft(), option.rect.topRight());
        gradient.setColorAt(0.0, highlight);
        gradient.setColorAt(stopPoint, rowColor);
        gradient.setColorAt(1.0, rowColor);
        painter->fillRect(option.rect, gradient);
    } else {
        painter->fillRect(option.rect, rowColor);
    }
    painter->save();

    const auto value = index.data(Qt::DisplayRole);
    if (value.isValid() && !value.isNull()) {
        const bool elideNeeded =
            initDocument(*document, option, value.toString());

        const int textPadding = 2;
        const auto deltaWidth = fixedOffset + textPadding;

        painter->translate(option.rect.topLeft() + QPoint(deltaWidth, 0));
        painter->setPen(option.palette.color(QPalette::Text));

        const QString ellipsis = QStringLiteral("…");
        const QFontMetrics fm(option.font);
        const int ellipsisWidth = fm.horizontalAdvance(ellipsis);
        const int ellipsisSpace = elideNeeded ? ellipsisWidth + 3 : 0;
        document->drawContents(
            painter, { 0, 0, option.rect.width() - deltaWidth - ellipsisSpace,
                       option.rect.height() });
        // Need to add ...
        if (elideNeeded) {
            const QPoint ellipsisPos(
                option.rect.width() - deltaWidth - ellipsisWidth, fm.ascent());
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
