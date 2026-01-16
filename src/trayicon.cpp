#include "trayicon.hpp"

#include "block_guard.hpp"
#include "configmanager.hpp"

#include <stdexcept>

#include <QAction>
#include <QApplication>
#include <QFont>
#include <QMenu>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <QString>
#include <QSvgRenderer>
#include <QSystemTrayIcon>
#include <QThread>

namespace
{
/// @brief This is render size for icon, tray will downscale to what it wants
/// later.
constexpr int kRenderSize = 256;
/// @brief Font size to use to draw emoji over icon.
constexpr int kEmojiFontSize = static_cast<int>(kRenderSize * 0.55);
} // namespace

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
    connect(tray_icon_menu_.get(), &QMenu::aboutToShow, this, [this]() {
        const auto noSignals = BlockGuard(mute_notifications_action_);
        mute_notifications_action_->setChecked(
            ConfigManager::config().get(ConfigManager::MuteNotifications));
    });

    updateStatusIcon(StatusEmoji::EmojiUrgency::None);
}

void SystemTrayIcon::updateStatusIcon(StatusEmoji::EmojiUrgency urgency)
{
    const bool muted =
        ConfigManager::config().get(ConfigManager::MuteNotifications);
    QPixmap pixmap(kRenderSize, kRenderSize);
    pixmap.fill(Qt::transparent);
    setToolTip(tr("There are no urgent tasks."));
    {
        QPainter painter(&pixmap);
        QSvgRenderer renderer(QStringLiteral(":/icons/qtask.svg"));
        renderer.render(&painter);

        // We need to update icon by emoji.
        if (urgency > StatusEmoji::EmojiUrgency::Future || muted) {
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);

            QFont font = painter.font();
            font.setPixelSize(kEmojiFontSize);
            painter.setFont(font);

            const QString emoji =
                muted ? (StatusEmoji::hasEmoji() ? "🔇" : QString())
                      : StatusEmoji::urgencyToEmoji(urgency);
            if (!emoji.isEmpty()) { // Check if we got support for emoji
                constexpr int badgeSize =
                    static_cast<int>(kEmojiFontSize * 1.2);
                constexpr int offset = kRenderSize - badgeSize;

                const QRect badgeRect(offset, offset, badgeSize, badgeSize);
                painter.drawText(badgeRect, Qt::AlignCenter, emoji);
            }

            setToolTip(muted ? tr("QTask - Notifications are muted.")
                             : tr("There are task(s) to deal with."));
        }
    } // painter.end() is called as out of scope

    setIcon(QIcon(pixmap));
}
