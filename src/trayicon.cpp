#include "trayicon.hpp"

#include <stdexcept>

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QObject>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QThread>

using namespace ui;

SystemTrayIcon::SystemTrayIcon(const bool isMuted, QObject *parent)
    : QSystemTrayIcon(parent)
    , tray_icon_menu_(new QMenu(nullptr))
    , mute_notifications_action_(
          new QAction(tr("&Mute Notifications"), tray_icon_menu_.get()))
{
    mute_notifications_action_->setCheckable(true);
    mute_notifications_action_->setChecked(isMuted);

    // tray_icon_menu_ takes ownership.
    const auto add_task_action_ =
        new QAction(tr("&Add Task"), tray_icon_menu_.get());
    const auto exit_action_ = new QAction(tr("&Quit"), tray_icon_menu_.get());

    tray_icon_menu_->addAction(add_task_action_);
    tray_icon_menu_->addSeparator();
    tray_icon_menu_->addAction(mute_notifications_action_);
    tray_icon_menu_->addAction(exit_action_);

    connect(mute_notifications_action_, &QAction::toggled, this,
            &SystemTrayIcon::muteNotificationsRequested);
    connect(add_task_action_, &QAction::triggered, this,
            &SystemTrayIcon::addTaskRequested);
    connect(exit_action_, &QAction::triggered, this,
            &SystemTrayIcon::exitRequested);

    setContextMenu(tray_icon_menu_.get());
    setIcon(QPixmap(":/icons/qtask.svg"));
    setToolTip(tr("QTask"));
}

bool SystemTrayIcon::isMuted() const
{
    auto *app = QApplication::instance();
    if (app && app->thread() == QThread::currentThread()) {
        return mute_notifications_action_->isChecked();
    }
    // FIXME/TODO: probably it is not issue below and it could be read from any
    // thread.
    throw std::runtime_error("Method should be called from GUI thread.");
}
