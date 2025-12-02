#include "taskhintproviderdelegate.hpp"
#include "async_task_loader.hpp"
#include "task.hpp"
#include "tasksmodel.hpp"

#include <QAbstractItemView>
#include <QHelpEvent>
#include <QModelIndex>
#include <QObject>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolTip>
#include <QVariant>
#include <QWidget>

#include <mutex>

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
                   ".tag-list { padding-left: 20px; margin: 0; }"
                   ".date-overdue { color: red; font-weight: bold; }"
                   ".date-approaching { color: orange; font-weight: bold; }"
                   ".date-normal { color: #555; }"
                   "</style>";

    const QDateTime now = QDateTime::currentDateTime();
    const qint64 oneDayMs = 24 * 60 * 60 * 1000;
    auto getDateCssClass = [&](const QDateTime &dt) -> QString {
        qint64 diffMs = now.msecsTo(dt);

        if (dt < now) {
            return "date-overdue";
        } else if (diffMs < oneDayMs) {
            return "date-approaching";
        }
        return "date-normal";
    };

    const QString &descriptionStr = task.description.get();
    if (!descriptionStr.isEmpty()) {
        html += QString("<h3>%1</h3>").arg(descriptionStr.toHtmlEscaped());
    } else {
        html += QString("<p style='color:#aaa;'>ID: %1</p>").arg(task.task_id);
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

    const bool hasDates = optionalDue.has_value() ||
                          optionalSched.has_value() || optionalWait.has_value();
    if (hasDates) {
        html += "<div class='info-block'><h3>Dates</h3>";

        // DUE DATE
        if (optionalDue.has_value()) {
            const QDateTime &dt = optionalDue.value();
            QString dateStr = dt.toString();
            QString cssClass = getDateCssClass(dt);

            html +=
                QString(
                    "<p class='date-due'><span class='%1'>Due: %2</span></p>")
                    .arg(cssClass)
                    .arg(dateStr);
        }

        // SCHED DATE
        if (optionalSched.has_value()) {
            const QDateTime &dt = optionalSched.value();
            QString dateStr = dt.toString();
            QString cssClass = getDateCssClass(dt);

            html += QString("<p><span class='%1'>Scheduled: %2</span></p>")
                        .arg(cssClass)
                        .arg(dateStr);
        }

        // WAIT DATE
        if (optionalWait.has_value()) {
            const QDateTime &dt = optionalWait.value();
            QString dateStr = dt.toString();
            QString cssClass = getDateCssClass(dt);

            html += QString("<p><span class='%1'>Wait until: %2</span></p>")
                        .arg(cssClass)
                        .arg(dateStr);
        }

        html += "</div>";
    }

    // ------------------------------------------------
    // TAGS (QStringList)
    // ------------------------------------------------
    const QStringList &tagsList = task.tags.get();
    if (!tagsList.isEmpty()) {
        html += "<div class='info-block'><h3>Tags</h3><ul class='tag-list'>";
        for (const QString &tag : tagsList) {
            html += QString("<li>%1</li>").arg(tag.toHtmlEscaped());
        }
        html += "</ul></div>";
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

    if (!task.isFullRead()) {
        const TaskLoaderParameters params{ index, view, event->globalPos() };
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
