#include "configmanager.hpp"

#include "iterable_per_string.hpp"

#include <QDebug>
#include <QDir>
#include <QObject>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace
{
const QString s_default_task_bin = "/usr/bin/task";

QString expandHome(const QString &p)
{
    if (p.startsWith(QString("~%1").arg(QDir::separator()))) {
        return QDir::homePath() + p.mid(1);
    }
    return p;
}

/// @brief New taskwarior uses SQL data base, config files and db could be
/// moved.
/// @returns path to folder which contains binary task data.
QString FindTaskDataFolder()
{
    static const std::vector<QString> kPossiblePath = {
        QString("%1%2%3%2").arg(QDir::homePath(), QDir::separator(), ".task"),
        QString("%1%2.config%2task%2taskrc")
            .arg(QDir::homePath(), QDir::separator()),
    };
    const auto it =
        std::find_if(kPossiblePath.begin(), kPossiblePath.end(),
                     [](const auto &str) { return QFile::exists(str); });
    if (it == kPossiblePath.end()) {
        std::cerr << "Could not find predefined files with settings. Using old "
                     "default value."
                  << std::endl;
        return kPossiblePath.front();
    }
    std::ifstream taskConf(it->toStdString());

    // Checking no more than linesToCheck inside the file if it has proper path.
    int linesToCheck = 100;
    for (const auto line : per_line_stream(taskConf)) // NOLINT
    {
        if (linesToCheck-- < 0) {
            std::cerr << "Nothing found in: " << it->toStdString() << std::endl;
            break;
        }
        auto keyValue = QString::fromStdString(line).split("=");
        keyValue.back() = expandHome(keyValue.back().trimmed());

        if (keyValue.size() == 2 &&
            keyValue.front().trimmed() == "data.location" &&
            QDir(keyValue.back()).exists()) {
            std::cout << "Found data location: "
                      << keyValue.back().toStdString() << std::endl;
            return std::move(keyValue.back());
        }
    }

    std::cerr << "Could not find data file location. Assuming old taskwarriror."
              << std::endl;
    return *it;
}
} // namespace

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , m_is_new(false)
    , m_config_path("")
    , m_task_bin(s_default_task_bin)
    , m_task_data_path(FindTaskDataFolder())
    , m_show_task_shell(false)
    , m_hide_on_startup(false)
    , m_save_filter_on_exit(false)
    , m_task_filter({})
{
}

ConfigManager &ConfigManager::config()
{
    static std::unique_ptr<ConfigManager> lazy_single_inst(new ConfigManager());
    return *lazy_single_inst;
}

bool ConfigManager::initializeFromFile()
{
    // QSettings ini(QString("%1%2%1").arg(arch), QSettings::IniFormat);

    // Try to locate existing config directory
    QVariant existsting_dir;
    for (const auto &p :
         QStandardPaths::standardLocations(QStandardPaths::ConfigLocation)) {
        QDir const d(p);
        if (d.exists("qtask")) {
            existsting_dir = QString("%1%2qtask").arg(p, QDir::separator());
            break;
        }
    }

    // Try to initialize a new config directory, if not found
    if (existsting_dir.isNull()) {
        for (const auto &p : QStandardPaths::standardLocations(
                 QStandardPaths::ConfigLocation)) {
            QDir const d(p);
            if (d.mkdir("qtask")) {
                existsting_dir = QString("%1%2qtask").arg(p, QDir::separator());
                break;
            }
        }
    }

    if (existsting_dir.isNull()) {
        return false;
    }

    QDir const config_dir(existsting_dir.toString());
    m_config_path = config_dir.absoluteFilePath("qtask.ini");
    if (!config_dir.exists("qtask.ini")) {
        if (!createNewConfigFile()) {
            return false;
        }
        m_is_new = true;
        return true;
    }

    return fillOptionsFromConfigFile();
}

void ConfigManager::updateConfigFile()
{
    QSettings settings(m_config_path, QSettings::IniFormat);
    if (!settings.isWritable()) {
        return;
    }
    settings.setValue("task_bin", m_task_bin);
    settings.setValue("task_data_path", m_task_data_path);
    settings.setValue("show_task_shell", m_show_task_shell);
    settings.setValue("hide_on_startup", m_hide_on_startup);
    settings.setValue("save_filter_on_exit", m_save_filter_on_exit);
    settings.setValue("task_filter", m_task_filter);
}

bool ConfigManager::createNewConfigFile()
{
    QSettings settings(m_config_path, QSettings::IniFormat);
    if (!settings.isWritable()) {
        return false;
    }
    settings.setValue("task_bin", s_default_task_bin);
    settings.setValue("task_data_path", FindTaskDataFolder());
    settings.setValue("show_task_shell", false);
    settings.setValue("hide_on_startup", false);
    settings.setValue("save_filter_on_exit", false);
    settings.setValue("task_filter", {});

    return true;
}

bool ConfigManager::fillOptionsFromConfigFile()
{
    const QSettings settings(m_config_path, QSettings::IniFormat);
    m_task_bin = settings.value("task_bin", s_default_task_bin).toString();
    m_task_data_path =
        settings.value("task_data_path", FindTaskDataFolder()).toString();
    m_show_task_shell = settings.value("show_task_shell", false).toBool();
    m_hide_on_startup = settings.value("hide_on_startup", false).toBool();
    m_save_filter_on_exit =
        settings.value("save_filter_on_exit", false).toBool();
    m_task_filter = settings.value("task_filter", "").toStringList();
    return true;
}
