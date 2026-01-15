#include "trayicon.hpp"

#include "block_guard.hpp"
#include "configmanager.hpp"

#include <stdexcept>

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QObject>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QThread>

using namespace ui;

SystemTrayIcon::SystemTrayIcon(QObject *parent)
    : QSystemTrayIcon(parent)
    , tray_icon_menu_(new QMenu(nullptr))
    , mute_notifications_action_(
          new QAction(tr("&Mute Notifications"), tray_icon_menu_.get()))
{
    mute_notifications_action_->setCheckable(true);
    mute_notifications_action_->setChecked(
        ConfigManager::config().get(ConfigManager::MuteNotifications));

    // tray_icon_menu_ takes ownership.
    const auto add_task_action_ =
        new QAction(tr("&Add Task"), tray_icon_menu_.get());
    const auto exit_action_ = new QAction(tr("&Quit"), tray_icon_menu_.get());

    tray_icon_menu_->addAction(add_task_action_);
    tray_icon_menu_->addSeparator();
    tray_icon_menu_->addAction(mute_notifications_action_);
    tray_icon_menu_->addAction(exit_action_);

    connect(mute_notifications_action_, &QAction::toggled, this,
            [this](bool checked) {
                ConfigManager::config().set(ConfigManager::MuteNotifications,
                                            checked);
            });
    connect(add_task_action_, &QAction::triggered, this,
            &SystemTrayIcon::addTaskRequested);
    connect(exit_action_, &QAction::triggered, this,
            &SystemTrayIcon::exitRequested);

    setContextMenu(tray_icon_menu_.get());
    setIcon(QPixmap(":/icons/qtask.svg"));
    setToolTip(tr("QTask"));

    connect(tray_icon_menu_.get(), &QMenu::aboutToShow, this, [this]() {
        const auto noSignals = BlockGuard(mute_notifications_action_);
        mute_notifications_action_->setChecked(
            ConfigManager::config().get(ConfigManager::MuteNotifications));
    });
}
