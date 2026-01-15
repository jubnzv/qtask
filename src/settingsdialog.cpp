#include "settingsdialog.hpp"

#include <QAbstractButton>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QWidget>

#include "configmanager.hpp"

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_task_bin_edit(new QLineEdit(this))
    , m_hide_on_startup_cb(
          new QCheckBox(tr("Hide QTask window on startup"), this))
    , m_save_filter_on_exit(new QCheckBox(tr("Save task filter on exit"), this))
    , m_buttons(new QDialogButtonBox(QDialogButtonBox::Ok |
                                         QDialogButtonBox::Apply |
                                         QDialogButtonBox::Close,
                                     this))

{
    setWindowTitle(tr("Settings"));
    setWindowIcon(QIcon(":/icons/qtask.svg"));

    initUI();
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::initUI()
{
    auto *main_layout = new QGridLayout(this);
    main_layout->setContentsMargins(5, 5, 5, 5);

    auto *task_bin_label = new QLabel(tr("task executable:"));
    main_layout->addWidget(task_bin_label, 0, 0);

    m_task_bin_edit->setText(
        ConfigManager::config().get(ConfigManager::TaskBin));
    main_layout->addWidget(m_task_bin_edit, 0, 1);

    auto *task_data_path_label = new QLabel(tr("Path to task data:"));
    main_layout->addWidget(task_data_path_label, 1, 0);

    m_hide_on_startup_cb->setChecked(
        ConfigManager::config().get(ConfigManager::HideWindowOnStartup));
    main_layout->addWidget(m_hide_on_startup_cb, 2, 0, 1, 2);

    m_save_filter_on_exit->setChecked(
        ConfigManager::config().get(ConfigManager::SaveFilterOnExit));
    main_layout->addWidget(m_save_filter_on_exit, 3, 0, 1, 2);

    connect(m_buttons, &QDialogButtonBox::clicked, this,
            &SettingsDialog::onButtonBoxClicked);
    main_layout->addWidget(m_buttons, 4, 0, 1, 2);

    setLayout(main_layout);
}

void SettingsDialog::applySettings()
{
    ConfigManager::config().set(ConfigManager::TaskBin,
                                m_task_bin_edit->text());
    ConfigManager::config().set(ConfigManager::HideWindowOnStartup,
                                m_hide_on_startup_cb->isChecked());
    ConfigManager::config().set(ConfigManager::SaveFilterOnExit,
                                m_save_filter_on_exit->isChecked());
    ConfigManager::config().updateConfigFile();
}

void SettingsDialog::onButtonBoxClicked(QAbstractButton *button)
{
    switch (m_buttons->standardButton(button)) {
    case QDialogButtonBox::Apply:
        applySettings();
        break;
    case QDialogButtonBox::Ok:
        applySettings();
        close();
    default:
        close();
    }
}
