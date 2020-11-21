#include "trayicon.hpp"

#include <QAction>
#include <QMenu>

using namespace ui;

SystemTrayIcon::SystemTrayIcon(QObject *parent)
    : QSystemTrayIcon(parent)
{
    tray_icon_menu_ = new QMenu();
    add_task_action_ = new QAction("Add &task");
    mute_notifications_action_ = new QAction("&Mute notifications");
    mute_notifications_action_->setCheckable(true);
    exit_action_ = new QAction("Quit");

    // tray_icon_menu_->addAction(mute_notifications_action_);
    // tray_icon_menu_->addSeparator();
    tray_icon_menu_->addAction(add_task_action_);
    tray_icon_menu_->addSeparator();
    tray_icon_menu_->addAction(exit_action_);

    connect(mute_notifications_action_, &QAction::triggered, this,
            &SystemTrayIcon::muteNotificationsRequested);
    connect(add_task_action_, &QAction::triggered, this,
            &SystemTrayIcon::addTaskRequested);
    connect(exit_action_, &QAction::triggered, this,
            &SystemTrayIcon::exitRequested);

    setContextMenu(tray_icon_menu_);
    setIcon(QIcon(":/icons/qtask.svg"));
    setToolTip("QTask");
}
