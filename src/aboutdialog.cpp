#include "aboutdialog.hpp"

#include <QDialog>
#include <QLabel>
#include <QObject>
#include <QString>
#include <QVBoxLayout>

#include "config.hpp"

AboutDialog::AboutDialog(const QVariant &task_version, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About"));
    initUI(task_version);
}

AboutDialog::~AboutDialog() = default;

void AboutDialog::initUI(const QVariant &task_version)
{

    const QString version = QString("%1.%2.%3")
                                .arg(QString::number(QTASK_VERSION_MAJOR),
                                     QString::number(QTASK_VERSION_MINOR),
                                     QString::number(QTASK_VERSION_PATCH));

    const QString info =
        QString(
            tr("<html><style>a { color: blue; text-decoration: none; "
               "}</style><body>"
               "<CENTER>"
               "<BR><IMG SRC=\":/icons/qtask.svg\">"
               "<P> Version <B>%1</B><BR>"
               "QTask is an open-source organizer based on Taskwarrior.<BR>"
               "<P> Components: Qt-%2,%3 <a "
               "href=\"https://smashicons.com/app/c/24px-icons/essentials_1/"
               "list/soft_outline\">Essential Icon Pack</a><P>"
               "<P><a "
               "Website: "
               "href=\"https://github.com/jubnzv/qtask\">https://github.com/"
               "jubnzv/qtask</a><P>"
               "<a E-mail: "
               "href=\"mailto:jubnzv@gmail.com\">jubnzv@gmail.com</a><P>"
               "</P>"
               "</CENTER></body></html>"))
            .arg(version, QT_VERSION_STR,
                 (task_version.isNull()
                      ? ""
                      : QString(tr("task %1,")).arg(task_version.toString())));

    auto *main_layout = new QVBoxLayout(this);
    auto *info_label = new QLabel(info, this);
    info_label->setOpenExternalLinks(true);
    info_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    main_layout->addWidget(info_label);

    setLayout(main_layout);
}
