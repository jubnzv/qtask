#ifndef SYSTRAYICON_HPP
#define SYSTRAYICON_HPP


#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QObject>

#include <memory>

namespace ui
{

class SystemTrayIcon : public QSystemTrayIcon {
    Q_OBJECT

  public:
    explicit SystemTrayIcon(QObject *parent);

  signals:
    void muteNotificationsRequested(bool value);
    void addTaskRequested();
    void exitRequested();

  private:
    std::unique_ptr<QMenu> tray_icon_menu_;
};

} // namespace ui

#endif // SYSTRAYICON_HPP
