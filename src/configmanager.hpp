#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include <QDir>
#include <QObject>
#include <QString>

class ConfigManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString m_task_bin READ getTaskBin WRITE setTaskBin NOTIFY
                   updateConfigFile)
    Q_PROPERTY(QString m_task_data_path READ getTaskBin WRITE setTaskBin NOTIFY
                   updateConfigFile)

  public:
    ConfigManager(QObject *parent = nullptr);
    ~ConfigManager() = default;

    static ConfigManager *config();

    bool isNew() const { return m_is_new; }

    QString getTaskBin() const { return m_task_bin; }
    void setTaskBin(const QString &v) { m_task_bin = v; }

    QString getTaskDataPath() const { return m_task_data_path; }
    void setTaskDataPath(const QString &v) { m_task_data_path = v; }

    bool initializeFromFile();

    void updateConfigFile();

  private:
    bool createNewConfigFile();
    bool fillOptionsFromConfigFile();

  private:
    static ConfigManager *inst_;

    /// Configuration file was created during initialization
    bool m_is_new;

    /// Path to configuration file
    QString m_config_path;

    QString m_task_bin;
    static const QString s_default_task_bin;
    QString m_task_data_path;
    static const QString s_default_task_data_path;
};

#endif // CONFIGMANAGER_HPP
