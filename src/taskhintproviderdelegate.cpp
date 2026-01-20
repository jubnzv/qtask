#include "taskhintproviderdelegate.hpp"
#include "async_task_loader.hpp"
#include "qtutil.hpp"
#include "task.hpp"
#include "task_date_time.hpp"
#include "task_emojies.hpp"
#include "tasksmodel.hpp"

#include <QAbstractItemView>
#include <QGuiApplication>
#include <QHelpEvent>
#include <QModelIndex>
#include <QObject>
#include <QStringList>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTimer>
#include <QToolTip>
#include <QVariant>
#include <QWidget>
#include <QtTypes>

#include <mutex>
#include <stdexcept>

namespace
{
QString getLoadingTooltip(const QString &footer)
{
    QString tooltip = QObject::tr("Loading...");
    if (!footer.isEmpty()) {
        tooltip += "\n";
        tooltip += footer;
    }
    return tooltip;
}

QString extractQtHtmlFragment(const QString &html)
{
    static const QString start = "<!--StartFragment-->";
    static const QString end = "<!--EndFragment-->";

    const auto s = html.indexOf(start);
    const auto e = html.indexOf(end);

    if (s == -1 || e == -1 || e <= s) {
        return {};
    }

    return html.mid(s + start.size(), e - (s + start.size()));
}

QString getDescriptionHtml(const QString &descr)
{
    QTextDocument doc;
    setContentOfTextDocument(doc, descr);

    const QTextDocumentFragment frag(&doc);
    return extractQtHtmlFragment(frag.toHtml());
}

QString toString(const QDateTime &dt)
{
    // Avoiding seconds in output.
    return QLocale::system().toString(dt, QLocale::ShortFormat);
}

QString generateTooltip(const DetailedTaskInfo &task, const QString &footer)
{
    if (!task.isFullRead()) {
        // Backup plan, possibly async failed to load data.
        return getLoadingTooltip(footer);
    }
    QString html = "<style>"
                   "h3 { margin: 0 0 3px 0; color: #34495e; }"
                   "p { margin: 2px 0; }"
                   ".info-block { border-top: 1px solid #eee; padding-top: "
                   "5px; margin-top: 5px; }"
                   ".priority-high { color: red; font-weight: bold; }"
                   ".date-due { color: #8e44ad; font-weight: bold; }"
                   ".date-overdue { color: red; font-weight: bold; }"
                   ".date-approaching { color: orange; font-weight: bold; }"
                   ".date-normal { color: #555; }"
                   ".date-active {color: #005fb8;font-weight: bold;}"
                   ".tag-list {"
                   "  list-style-type: none;"
                   "  padding: 0;"
                   "  margin: 0;"
                   "  display: flex;"
                   "  flex-wrap: wrap;"
                   "}"
                   ".tag-list li {"
                   "  background: #f0f0f0;"
                   "  padding: 2px 6px;"
                   "  margin: 2px;"
                   "  display: inline-block;"
                   "  color: #333;"
                   "  font-size: 11px;"
                   "}"
                   ".read-only-banner {"
                   "  background: #fff3cd;"
                   "  border: 1px solid #ffeeba;"
                   "  color: #856404;"
                   "  padding: 5px;"
                   "  margin: 5px 0;"
                   "  border-radius: 3px;"
                   "  font-weight: bold;"
                   "}"
                   ".period-val { font-style: italic; color: #533f03; }"
                   "</style>";

    static const auto getDateCssClass = [](const auto &dt) -> QString {
        switch (dt.relationToNow()) {
        case DatesRelation::Past:
            return "date-overdue";
        case DatesRelation::Approaching:
            return "date-approaching";
        case DatesRelation::Future:
            return "date-normal";
        }
        throw std::runtime_error(
            "Not handled TaskDateTime::DateRelationToNow value.");
    };

    const QString &descriptionStr = task.description.get();
    if (!descriptionStr.isEmpty()) {
        html += QString("<h3>%1</h3>").arg(getDescriptionHtml(descriptionStr));
    } else {
        html += QString("<p style='color:#aaa;'>ID: %1</p>").arg(task.task_id);
    }

    // ------------------------------------------------
    // RECURRENCE LOCK WARNING
    // ------------------------------------------------
    if (task.reccurency_period.get().isRecurrent()) {
        QString message = "⚠️ Recurring Instance (Read Only)";

        if (auto p = task.reccurency_period.get().period()) {
            message +=
                QString("<br/><span class='period-val'>(Period: %1)</span>")
                    .arg(*p);
        }

        html += QString("<div class='read-only-banner'>%1</div>").arg(message);
    }

    // ------------------------------------------------
    // PROJECT
    // ------------------------------------------------
    html += "<div class='info-block'>";

    // Project (QString)
    if (!task.project.get().isEmpty()) {
        html += QString("<p><b>Project:</b> <i>%1</i></p>")
                    .arg(task.project.get().toHtmlEscaped());
    }

    html += "</div>";

    // ------------------------------------------------
    // DATES (std::optional<QDateTime>)
    // ------------------------------------------------
    const auto &optionalDue = task.due.get();
    const auto &optionalSched = task.sched.get();
    const auto &optionalWait = task.wait.get();
    const StatusEmoji status(task);

    if (status.isSpecialSched()) {
        html += "<div class='info-block'>";
        html += "<p><span class='date-active'>&nbsp;In Progress</span></p>";
        html += "</div>";
    }

    const bool hasDates = optionalDue.has_value() ||
                          optionalSched.has_value() || optionalWait.has_value();
    if (hasDates) {
        html += "<div class='info-block'><h3>Dates</h3>";

        // SCHED DATE
        if (optionalSched.has_value()) {
            const QString dateStr = toString(optionalSched.value());
            const QString cssClass = getDateCssClass(optionalSched);
            html +=
                QString("<p><span class='%1'>%3&nbsp;Scheduled: %2</span></p>")
                    .arg(cssClass, dateStr, status.schedEmoji());
        }

        // DUE DATE
        if (optionalDue.has_value()) {
            const QString dateStr = toString(optionalDue.value());
            const QString cssClass = getDateCssClass(optionalDue);
            html += QString("<p class='date-due'><span class='%1'>%3&nbsp;Due: "
                            "%2</span></p>")
                        .arg(cssClass, dateStr, status.dueEmoji());
        }

        // WAIT DATE
        if (optionalWait.has_value()) {
            const QString dateStr = toString(optionalWait.value());
            const QString cssClass = getDateCssClass(optionalWait);
            html +=
                QString("<p><span class='%1'>%3&nbsp;Wait until: %2</span></p>")
                    .arg(cssClass, dateStr, status.waitEmoji());
        }

        html += "</div>";
    }

    // ------------------------------------------------
    // TAGS (QStringList)
    // ------------------------------------------------
    const auto &tagsList = task.tags.get();
    if (!tagsList.isEmpty()) {
        html += "<div class='info-block'><h3>Tags</h3><p>";
        html += tagsList.join(", ");
        html += "</p></div>";
    }

    // ------------------------------------------------
    // FOOTER
    // ------------------------------------------------
    if (!footer.isEmpty()) {
        html += QString("<div class='info-block' style='color:#888;'>%1</div>")
                    .arg(footer.toHtmlEscaped());
    }

    return html;
}

} // namespace

TaskHintProviderDelegate::TaskHintProviderDelegate(QObject *parent)
    : QStyledItemDelegate{ parent }
    , m_task_loader(AsyncTaskLoader::create(this))
{
    static std::once_flag metaTypeRegistrationFlag;
    std::call_once(metaTypeRegistrationFlag, []() {
        qRegisterMetaType<TaskLoaderParameters>("TaskLoaderParameters");
    });

    connect(m_task_loader, &AsyncTaskLoader::latestLoadingFinished, this,
            [this](const QVariant &varParams, [[maybe_unused]] int requestId,
                   const DetailedTaskInfo &fullTask) {
                if (!varParams.canConvert<TaskLoaderParameters>()) {
                    return;
                }
                auto params = varParams.value<TaskLoaderParameters>();
                if (params.view) {
                    if (params.index.isValid()) {
                        params.view->model()->setData(
                            params.index, QVariant::fromValue(fullTask),
                            TasksModel::TaskUpdateRole);
                    }
                    // We got good response, cancel "Loading" timer to avoid
                    // blinking.
                    m_pendingLoadingRequestId.store(
                        AsyncTaskLoader::kInvalidRequestId);
                    QToolTip::showText(
                        params.cursor_pos,
                        generateTooltip(fullTask, getToolTipFooter()),
                        params.view);
                }
            });
}

bool TaskHintProviderDelegate::helpEvent(QHelpEvent *event,
                                         QAbstractItemView *view,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index)
{
    if (event->type() != QEvent::ToolTip || !index.isValid()) {
        return QStyledItemDelegate::helpEvent(event, view, option, index);
    }
    const auto variantTask =
        index.model()->data(index, TasksModel::TaskReadRole);

    if (!variantTask.isValid()) {
        QToolTip::hideText();
        return true;
    }
    const auto task = variantTask.value<DetailedTaskInfo>();
    const TaskLoaderParameters params{ index, view, event->globalPos() };

    if (task.isFullRead()) {
        QToolTip::showText(params.cursor_pos,
                           generateTooltip(task, getToolTipFooter()),
                           params.view);
    } else {
        const auto currentRequestId =
            m_task_loader->startTaskLoad(params, task);
        // It is safe, because lambda which will write it is in the same GUI
        // thread.
        m_pendingLoadingRequestId.store(currentRequestId);
        QTimer::singleShot(400, this, [this, params, currentRequestId]() {
            // If it is the same value as before singleShot, then async response
            // didn't reset it yet.
            if (m_pendingLoadingRequestId.load() == currentRequestId) {
                m_pendingLoadingRequestId.store(
                    AsyncTaskLoader::kInvalidRequestId);
                if (params.view) {
                    QToolTip::showText(params.cursor_pos,
                                       getLoadingTooltip(getToolTipFooter()),
                                       params.view);
                }
            }
        });
    }

    return true;
}

const QString &TaskHintProviderDelegate::getToolTipFooter() const
{
    static const QString hint(
        tr("Right-click in project col to add to filters."));
    return hint;
}

QColor TaskHintProviderDelegate::getHighlightColor(
    const QStyleOptionViewItem &option) const
{
    QColor highlight = option.palette.highlight().color();
    if (qApp->palette().color(QPalette::Base).value() < 128) {
        highlight = highlight.lighter(130);
    }
    return highlight;
}
