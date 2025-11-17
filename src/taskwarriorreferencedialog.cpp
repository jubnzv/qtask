#include "taskwarriorreferencedialog.hpp"

#include <QApplication>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QDebug>
#include <QLabel>
#include <QPalette>

TaskwarriorReferenceDialog::TaskwarriorReferenceDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Taskwarrior Quick Reference"));
    initUI();
}

TaskwarriorReferenceDialog::~TaskwarriorReferenceDialog() = default;

void TaskwarriorReferenceDialog::initUI()
{

    const auto group = QPalette::Active;
    const auto role = QPalette::Window;

    auto palette = QApplication::palette();
    QColor c1 = palette.color(group, role).name();
    c1.setGreen(c1.green() + 10);
    QColor c2 = palette.color(group, role).name();
    c2.setBlue(c2.blue() + 10);
    QColor c3 = palette.color(group, role).name();
    c3.setRed(c3.red() + 10);

    const QString info =
        QString("<html><style>a { color: blue; text-decoration: none; "
                "}</style><body>"
                ""
                "<table style=\"padding: 5px;\">"
                "<tr><td>"
                "<div style=\"background-color: %1;\">"
                "<h3>Filters</h3>"
                "<table style=\"background-color: %1;\">"
                "<tr><td><code>pro:project_name</code></td></tr>"
                "<tr><td><code>pro.not:project_name</code></td></tr>"
                "<tr><td><code>+tag_name</code></td></tr>"
                "<tr><td><code>-tag_name</code></td></tr>"
                "</table>"
                "</div>"
                "</td>"
                ""
                "<td>"
                "<div style=\"background-color: %2;\">"
                "<h3>Virtual tags</h3>"
                "<table style=\"background-color: %2;\">"
                "<tr><td><code>+DUE</code></td></tr>"
                "<tr><td><code>+DUETODAY</code></td></tr>"
                "<tr><td><code>+OVERDUE</code></td></tr>"
                "<tr><td><code>+WEEK</code></td></tr>"
                "<tr><td><code>+MONTH</code></td></tr>"
                "<tr><td><code>+YEAR</code></td></tr>"
                "<tr><td><code>+ACTIVE</code></td></tr>"
                "<tr><td><code>+SCHEDULED</code></td></tr>"
                "<tr><td><code>+UNTIL</code></td></tr>"
                "<tr><td><code>+WAITING</code></td></tr>"
                "<tr><td><code>+ANNOTATED</code></td></tr>"
                "</div>"
                "</table>"
                "</td></tr>"
                ""
                "</table>"
                "</body></html>")
            .arg(c1.name(), c2.name());

    auto *main_layout = new QVBoxLayout(this);
    auto *info_label = new QLabel(info, this);
    info_label->setOpenExternalLinks(true);
    info_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    main_layout->addWidget(info_label);

    setLayout(main_layout);
}
