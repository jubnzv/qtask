#ifndef SYSTRAYICON_HPP
#define SYSTRAYICON_HPP

#include "task_emojies.hpp"

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>

#include <memory>

namespace ui
{

class SystemTrayIcon : public QSystemTrayIcon {
    Q_OBJECT

  public:
    explicit SystemTrayIcon(QObject *parent);
  public slots:
    void updateStatusIcon(StatusEmoji::EmojiUrgency urgency);
  signals:
    void addTaskRequested();
    void exitRequested();

  private:
    // We must use unique_ptr, because QSystemTrayIcon is NOT QWidget, while
    // menu would expect one.
    std::unique_ptr<QMenu> tray_icon_menu_;
    QAction *mute_notifications_action_;
};

} // namespace ui

#endif // SYSTRAYICON_HPP
