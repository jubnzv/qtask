#include <QApplication>
#include <QMessageBox>
#include <QObject>

#include "configmanager.hpp"
#include "mainwindow.hpp"

using namespace ui;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("JTask");

    if (!ConfigManager::config()->initializeFromFile()) {
        QMessageBox::warning(
            nullptr, QObject::tr("Warning"),
            QObject::tr("Can't read configuration file. The default "
                        "settings will be used."));
    }

    MainWindow main_win;
    main_win.resize(700, 200);
    main_win.show();

    int rc = a.exec();
    ConfigManager::config()->updateConfigFile();
    return rc;
}
