#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QMessageBox>
#include <QObject>
#include <QStringList>

#include "config.hpp"
#include "configmanager.hpp"
#include "mainwindow.hpp"
#include "singleapplication.h"
#include "taskdialog.hpp"

using namespace ui;

int main(int argc, char *argv[])
{
    SingleApplication app(argc, argv, /*allowSecondary=*/true,
                          SingleApplication::Mode::User |
                              SingleApplication::Mode::SecondaryNotification);
    app.setQuitOnLastWindowClosed(false); // don't close app in tray
    app.setApplicationName("QTask");
    app.setApplicationVersion(QString("%1.%2.%3")
                                  .arg(QString::number(QTASK_VERSION_MAJOR),
                                       QString::number(QTASK_VERSION_MINOR),
                                       QString::number(QTASK_VERSION_PATCH)));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        "An open-source organizer based on Taskwarrior.");
    parser.addHelpOption();

    QCommandLineOption add_task_option(
        "a", "Add a task without starting a new instance.");
    parser.addOption(add_task_option);

    if (!ConfigManager::config()->initializeFromFile()) {
        QMessageBox::warning(
            nullptr, QObject::tr("Warning"),
            QObject::tr("Can't read configuration file. The default "
                        "settings will be used."));
    }

    parser.process(app);

    if (parser.isSet(add_task_option)) {
        if (app.isSecondary()) {
            app.sendMessage("add_task");
            return EXIT_SUCCESS;
        } else {
            auto *dlg = new AddTaskDialog();
            if (dlg->exec() != QDialog::Accepted)
                return EXIT_SUCCESS;
            auto t = dlg->getTask();
            Taskwarrior task_provider;
            return (task_provider.addTask(t)) ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

    MainWindow main_win;
    main_win.resize(700, 200);

    // If this is a secondary instance
    if (app.isSecondary()) {
        qDebug() << "QTask already running.";
        qDebug() << "Primary instance PID: " << app.primaryPid();
        qDebug() << "Primary instance user: " << app.primaryUser();
        app.sendMessage("show");
        return EXIT_SUCCESS;
    } else {
        QObject::connect(&app, &SingleApplication::receivedMessage, &main_win,
                         &MainWindow::receiveNewInstanceMessage);
    }

    int rc = app.exec();
    ConfigManager::config()->updateConfigFile();
    return rc;
}
