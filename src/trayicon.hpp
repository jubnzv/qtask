#ifndef SYSTRAYICON_HPP
#define SYSTRAYICON_HPP

#include <memory>

#include <QSystemTrayIcon>

class QAction;
class QMenu;
class QObject;

namespace ui
{

class SystemTrayIcon : public QSystemTrayIcon {
    Q_OBJECT

  public:
    SystemTrayIcon(QObject *parent);

  signals:
    void muteNotificationsRequested(bool value);
    void addTaskRequested();
    void exitRequested();

  private:
    QMenu *tray_icon_menu_;
    QAction *add_task_action_;
    QAction *mute_notifications_action_;
    QAction *exit_action_;
};

} // namespace ui

#endif // SYSTRAYICON_HPP
