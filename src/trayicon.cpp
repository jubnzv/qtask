#include "trayicon.hpp"

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QPixmap>
#include <QSystemTrayIcon>

using namespace ui;

SystemTrayIcon::SystemTrayIcon(QObject *parent)
    : QSystemTrayIcon(parent)
    , tray_icon_menu_(new QMenu())
{
    // tray_icon_menu_ takes ownership.
    const auto add_task_action_ =
        new QAction(tr("Add &task"), tray_icon_menu_.get());
    const auto mute_notifications_action_ =
        new QAction(tr("&Mute notifications"), tray_icon_menu_.get());
    const auto exit_action_ = new QAction(tr("&Quit"), tray_icon_menu_.get());

    tray_icon_menu_->addAction(add_task_action_);
    tray_icon_menu_->addSeparator();
    tray_icon_menu_->addAction(mute_notifications_action_);
    tray_icon_menu_->addAction(exit_action_);

    connect(mute_notifications_action_, &QAction::triggered, this,
            &SystemTrayIcon::muteNotificationsRequested);
    connect(add_task_action_, &QAction::triggered, this,
            &SystemTrayIcon::addTaskRequested);
    connect(exit_action_, &QAction::triggered, this,
            &SystemTrayIcon::exitRequested);

    setContextMenu(tray_icon_menu_.get());
    setIcon(QPixmap(":/icons/qtask.svg"));
    setToolTip(tr("QTask"));
}
