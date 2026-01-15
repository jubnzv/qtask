#include "settingsdialog.hpp"
#include "block_guard.hpp"
#include "configmanager.hpp"
#include "lambda_visitors.hpp"

#include <variant>

#include <QAbstractButton>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_task_bin_edit(new QLineEdit(this))
    , m_hide_on_startup_cb(
          new QCheckBox(tr("Hide QTask window on startup"), this))
    , m_save_filter_on_exit(new QCheckBox(tr("Save task filter on exit"), this))
    , m_mute_notifications_cb(new QCheckBox(tr("Mute notifications"), this))
    , m_buttons(new QDialogButtonBox(QDialogButtonBox::Ok |
                                         QDialogButtonBox::Apply |
                                         QDialogButtonBox::Close,
                                     this))
    , binders{ { ConfigManager::TaskBin, m_task_bin_edit },
               { ConfigManager::HideWindowOnStartup, m_hide_on_startup_cb },
               { ConfigManager::SaveFilterOnExit, m_save_filter_on_exit },
               { ConfigManager::MuteNotifications, m_mute_notifications_cb } }

{
    setWindowTitle(tr("Settings"));
    setWindowIcon(QIcon(":/icons/qtask.svg"));

    initUI();
    updateButtonsState();
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::initUI()
{
    auto *main_layout = new QGridLayout(this);
    main_layout->setContentsMargins(12, 12, 12, 12);
    main_layout->setSpacing(10);

    // Path
    auto *task_bin_label = new QLabel(tr("Task executable:"));
    main_layout->addWidget(task_bin_label, 0, 0);
    main_layout->addWidget(m_task_bin_edit, 0, 1);
    main_layout->addWidget(m_hide_on_startup_cb, 1, 0, 1, 2);
    main_layout->addWidget(m_save_filter_on_exit, 2, 0, 1, 2);
    main_layout->addWidget(m_mute_notifications_cb, 3, 0, 1, 2);

    // Spacer
    main_layout->setRowMinimumHeight(4, 10);

    // Buttons
    connect(m_buttons, &QDialogButtonBox::clicked, this,
            &SettingsDialog::onButtonBoxClicked);
    main_layout->addWidget(m_buttons, 5, 0, 1, 2);

    setLayout(main_layout);

    // Tray icon menu could change value.
    connect(&ConfigManager::config().notifier(), &ConfigEvents::settingsChanged,
            this, [this]() {
                auto guard = BlockGuard(m_mute_notifications_cb);
                m_mute_notifications_cb->setChecked(ConfigManager::config().get(
                    ConfigManager::MuteNotifications));
                updateButtonsState();
            });

    for (const auto &b : binders) {
        const LambdaVisitor visitor{
            [&b, this](QCheckBox *checkbox) {
                checkbox->setChecked(
                    ConfigManager::config().get_as<bool>(b.key));
                connect(checkbox, &QCheckBox::toggled, this,
                        &SettingsDialog::updateButtonsState);
            },
            [&b, this](QLineEdit *edit) {
                edit->setText(
                    ConfigManager::config().get_as<QString>(b.key));
                connect(edit, &QLineEdit::textChanged, this,
                        &SettingsDialog::updateButtonsState);
            },
        };
        std::visit(visitor, b.widget);
    }
}

void SettingsDialog::applySettings()
{
    auto &cfg = ConfigManager::config();
    for (const auto &b : binders) {
        const LambdaVisitor visitor{
            [&b, &cfg](QCheckBox *checkbox) {
                cfg.set_as(b.key, checkbox->isChecked());
            },
            [&b, &cfg](QLineEdit *edit) { cfg.set_as(b.key, edit->text()); },
        };
        std::visit(visitor, b.widget);
    }
    cfg.updateConfigFile();
    updateButtonsState();
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
        break;
    default:
        close();
        break;
    }
}

void SettingsDialog::updateButtonsState()
{
    bool changed = false;
    for (const auto &b : binders) {
        changed |= std::visit(
            LambdaVisitor{
                [&](QCheckBox *cb) {
                    return cb->isChecked() !=
                           ConfigManager::config().get_as<bool>(b.key);
                },
                [&](QLineEdit *le) {
                    return le->text() !=
                           ConfigManager::config().get_as<QString>(b.key);
                } },
            b.widget);

        if (changed) {
            break;
        }
    }
    m_buttons->button(QDialogButtonBox::Apply)->setEnabled(changed);
    m_buttons->button(QDialogButtonBox::Ok)->setEnabled(changed);
}
