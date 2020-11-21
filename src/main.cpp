#include <QApplication>
#include <QDebug>
#include <QMessageBox>
#include <QObject>

#include "configmanager.hpp"
#include "mainwindow.hpp"
#include "singleapplication.h"

using namespace ui;

int main(int argc, char *argv[])
{
    SingleApplication app(argc, argv, /*allowSecondary=*/false,
                          SingleApplication::Mode::User |
                              SingleApplication::Mode::SecondaryNotification);
    app.setApplicationName("JTask");

    if (!ConfigManager::config()->initializeFromFile()) {
        QMessageBox::warning(
            nullptr, QObject::tr("Warning"),
            QObject::tr("Can't read configuration file. The default "
                        "settings will be used."));
    }

    MainWindow main_win;
    main_win.resize(700, 200);
    main_win.show();

    // If this is a secondary instance
    if (app.isSecondary()) {
        app.sendMessage(app.arguments().join(' ').toUtf8());
        qDebug() << "JTask already running.";
        qDebug() << "Primary instance PID: " << app.primaryPid();
        qDebug() << "Primary instance user: " << app.primaryUser();
        return EXIT_SUCCESS;
    } else {
        QObject::connect(&app, &SingleApplication::instanceStarted, &main_win,
                         &MainWindow::showMainWindow);
    }

    int rc = app.exec();
    ConfigManager::config()->updateConfigFile();
    return rc;
}
