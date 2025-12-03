#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include <QDir>
#include <QObject>
#include <QString>

class ConfigManager : public QObject {
  private:
    Q_OBJECT
    ConfigManager(QObject *parent = nullptr);

  public:
    ~ConfigManager() = default;
    static ConfigManager &config();

    bool isNew() const { return m_is_new; }

    const QString &getTaskBin() const { return m_task_bin; }
    void setTaskBin(const QString &v) { m_task_bin = v; }

    bool getShowTaskShell() const { return m_show_task_shell; }
    void setShowTaskShell(bool v) { m_show_task_shell = v; }

    bool getHideWindowOnStartup() const { return m_hide_on_startup; }
    void setHideWindowOnStartup(bool v) { m_hide_on_startup = v; }

    bool getSaveFilterOnExit() const { return m_save_filter_on_exit; }
    void setSaveFilterOnExit(bool v) { m_save_filter_on_exit = v; }

    QStringList getTaskFilter() const { return m_task_filter; }
    void setTaskFilter(const QStringList &v) { m_task_filter = v; }

    bool initializeFromFile();

    void updateConfigFile();

  private:
    bool createNewConfigFile();
    bool fillOptionsFromConfigFile();

  private:
    /// Configuration file was created during initialization
    bool m_is_new;

    /// Path to configuration file
    QString m_config_path;

    /// Path to task binary
    QString m_task_bin;

    /// Task shell will be shown in the main window
    bool m_show_task_shell;

    /// QTask window is hidden on startup
    bool m_hide_on_startup;

    /// QTask will save current task filter on exit and apply it after restart.
    bool m_save_filter_on_exit;

    /// task filter from the previous launch.
    QStringList m_task_filter;
};

#endif // CONFIGMANAGER_HPP
