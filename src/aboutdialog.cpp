#include "aboutdialog.hpp"

#include <QCoreApplication>
#include <QDialog>
#include <QLabel>
#include <QObject>
#include <QString>
#include <QVBoxLayout>

#include "config.hpp"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    initUI();
}

void AboutDialog::initUI()
{
    setWindowTitle(QCoreApplication::applicationName() + " - About");

    const QString version = QString("%1.%2.%3")
                                .arg(QString::number(JTASK_VERSION_MAJOR),
                                     QString::number(JTASK_VERSION_MINOR),
                                     QString::number(JTASK_VERSION_PATCH));

    const QString info =
        QString("<html><style>a { color: blue; text-decoration: none; "
                "}</style><body>"
                "<CENTER>"
                "<BR><IMG SRC=\":/img/jtask.svg\">"
                "<P> Version <B>%1</B><BR>"
                "JTask is an open-source organizer based on taskwarrior.<BR>"
                "<P> Components: Qt-%2, <a "
                "href=\"https://smashicons.com/app/c/24px-icons/essentials_1/"
                "list/soft_outline\">Essential Icon Pack</a><P>"
                "<P><a "
                "Website: "
                "href=\"https://github.com/jubnzv/jtask\">https://github.com/"
                "jubnzv/jtask</a><P>"
                "<a E-mail: "
                "href=\"mailto:jubnzv@gmail.com\">jubnzv@gmail.com</a><P>"
                "</P>"
                "</CENTER></body></html>")
            .arg(version, QT_VERSION_STR);

    QVBoxLayout *main_layout = new QVBoxLayout();
    QLabel *info_label = new QLabel(info);
    info_label->setOpenExternalLinks(true);
    info_label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    main_layout->addWidget(info_label);

    setLayout(main_layout);
}
