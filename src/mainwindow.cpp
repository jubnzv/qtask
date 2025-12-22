#include "mainwindow.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QEvent>
#include <QGridLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QObject>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QTableView>
#include <QToolBar>
#include <QVariant>
#include <QWindowStateChangeEvent>

#include <qassert.h>
#include <qnamespace.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "aboutdialog.hpp"
#include "agendadialog.hpp"
#include "configmanager.hpp"
#include "datetimedialog.hpp"
#include "qtutil.hpp"
#include "recurringdialog.hpp"
#include "settingsdialog.hpp"
#include "tagsedit.hpp"
#include "task.hpp"
#include "taskdescriptiondelegate.hpp"
#include "taskdialog.hpp"
#include "tasksmodel.hpp"
#include "tasksview.hpp"
#include "taskwarrior.hpp"
#include "taskwarriorreferencedialog.hpp"
#include "taskwatcher.hpp"
#include "trayicon.hpp"

#include <cassert>
#include <map>
#include <memory>
#include <optional>
#include <utility>

using namespace ui;

namespace
{
const auto kStartStopShortcut = QKeySequence("CTRL+S");
const auto kSettingsButtonShortcut = QKeySequence("CTRL+SHIFT+S");
const auto kActivateTaskShellShortcut = QKeySequence("CTRL+T");
const auto kDoneShortcut = QKeySequence("CTRL+D");
const auto kEditShortcut = QKeySequence("CTRL+E");
const auto kWaitShortcut = QKeySequence("CTRL+W");
const auto kAgendaViewShortcut = QKeySequence("ALT+A");
const auto kRecurrentViewShortcut = QKeySequence("ALT+R");

} // namespace

MainWindow::MainWindow()
    : m_window_prev_state(Qt::WindowNoState)
    , m_is_quit(false)
    , m_window(new QWidget(this))
    , m_layout(new QGridLayout(m_window))
    , m_tray_icon(new SystemTrayIcon(this))
    , m_tasks_view(new TasksView(m_window))
    , m_task_toolbar(new QToolBar(tr("Task Toolbar"), m_window))
    , m_task_shell(new QLineEdit(m_window))
    , m_task_filter(new TagsEdit(m_window))
    , m_view_menu_actions(*menuBar()->addMenu(tr("&View")))
    , m_toolbar_actions(*m_task_toolbar)
    , m_task_provider(std::make_unique<Taskwarrior>())
    , m_task_watcher(new TaskWatcher(this))
{
    if (!m_task_provider->init()) {
        QMessageBox::critical(
            this, tr("Error"),
            tr("Command 'task version' failed. Please make sure that "
               "taskwarrior is installed correctly and you have the correct "
               "path to the 'task' executable in the settings."));
    }

    // Tags must be loaded before any events happened so it could setup
    // "modified" tracker.
    if (ConfigManager::config().getSaveFilterOnExit()) {
        auto tags = ConfigManager::config().getTaskFilter();
        tags.removeAll(QString(""));
        if (!tags.isEmpty()) {
            m_task_filter->setTags(tags);
        }
    }

    initTaskWatcher();
    initMainWindow();
    initTrayIcon();
    initFileMenu();
    connectViewMenuActions();
    initToolsMenu();
    initHelpMenu();
    initShortcuts();

    (ConfigManager::config().getHideWindowOnStartup()) ? hide() : show();

    QTimer::singleShot(5000, this, &MainWindow::onApplyFilter);
}

MainWindow::~MainWindow() { m_task_provider.reset(); }

bool MainWindow::initTaskWatcher()
{
    connect(
        m_task_watcher, &TaskWatcher::dataOnDiskWereChanged, this, [this]() {
            if (m_task_provider) {
                auto tasks = m_task_provider->getTasks();
                auto *model = qobject_cast<TasksModel *>(m_tasks_view->model());
                model->setTasks(tasks.value_or({}), getSelectedTaskIds());

                updateTaskToolbar();
            }
        });
    return true;
}

void MainWindow::initMainWindow()
{
    setWindowIcon(QIcon(":/icons/qtask.svg"));
    setWindowTitle(QCoreApplication::applicationName());
    setMinimumSize(400, 500);

    connectTaskToolbarActions();
    initTasksTable();

    m_task_shell->addAction(QIcon(":/icons/taskwarrior.png"),
                            QLineEdit::LeadingPosition);
    connect(m_task_shell, &QLineEdit::returnPressed, this,
            &MainWindow::onEnterTaskCommand);
    m_task_shell->setVisible(ConfigManager::config().getShowTaskShell());

    connect(m_task_filter, &TagsEdit::tagsChanged, this,
            &MainWindow::onApplyFilter);

    m_layout->addWidget(m_task_toolbar, 0, 0);
    m_layout->addWidget(m_task_filter, 0, 1);
    m_layout->addWidget(m_tasks_view, 1, 0, 1, 2);
    m_layout->addWidget(m_task_shell, 2, 0, 1, 2);

    m_window->setLayout(m_layout);
    setCentralWidget(m_window);

    refreshTasksListTableIfNeeded();
}

void MainWindow::initTasksTable()
{
    m_tasks_view->setShowGrid(true);

    m_tasks_view->verticalHeader()->setVisible(false);
    m_tasks_view->horizontalHeader()->setStretchLastSection(true);
    m_tasks_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    auto *model = new TasksModel(this);
    m_tasks_view->setModel(model);

    // All show hint, description column has additional logic too.
    m_tasks_view->setItemDelegate(new TaskHintProviderDelegate(m_tasks_view));
    m_tasks_view->setItemDelegateForColumn(
        2 /* description */, new TaskDescriptionDelegate(m_tasks_view));

    connect(m_tasks_view->selectionModel(),
            &QItemSelectionModel::selectionChanged, this,
            &MainWindow::updateTaskToolbar);
    connect(m_tasks_view, &TasksView::pushProjectFilter, this,
            &MainWindow::pushFilterTag);
    connect(m_tasks_view, &QTableView::doubleClicked, this,
            &MainWindow::showEditTaskDialog);
    connect(m_tasks_view, &TasksView::linkActivated, this,
            [&](const QString &link) {
                if (!link.isEmpty()) {
                    QDesktopServices::openUrl(link);
                }
            });
    connect(m_tasks_view, &TasksView::selectedTaskIsActive, this,
            [&](bool is_active) {
                if (is_active) {
                    removeShortcutFromToolTip(m_toolbar_actions.m_start_action);
                    removeShortcutFromToolTip(m_toolbar_actions.m_stop_action);
                    m_toolbar_actions.m_stop_action->setShortcut(
                        kStartStopShortcut);
                    addShortcutToToolTip(m_toolbar_actions.m_stop_action);
                    m_toolbar_actions.m_start_action->setEnabled(false);
                    m_toolbar_actions.m_stop_action->setEnabled(true);
                } else {
                    removeShortcutFromToolTip(m_toolbar_actions.m_stop_action);
                    removeShortcutFromToolTip(m_toolbar_actions.m_start_action);
                    m_toolbar_actions.m_start_action->setShortcut(
                        kStartStopShortcut);
                    addShortcutToToolTip(m_toolbar_actions.m_start_action);
                    m_toolbar_actions.m_start_action->setEnabled(true);
                    m_toolbar_actions.m_stop_action->setEnabled(false);
                }
            });

    // Support for restoring selection after model reset.
    connect(model, &TasksModel::selectIndices, this,
            [this](const QModelIndexList &indices) {
                m_tasks_view->selectionModel()->clearSelection();
                for (const auto &index : indices) {
                    m_tasks_view->selectionModel()->select(
                        index, QItemSelectionModel::Select |
                                   QItemSelectionModel::Rows);
                }
                // Scroll to the 1st selected (UX).
                if (!indices.isEmpty()) {
                    m_tasks_view->scrollTo(indices.first());
                }
            });

    m_tasks_view->installEventFilter(this);
}

void MainWindow::initTrayIcon()
{
    connect(m_tray_icon, &SystemTrayIcon::muteNotificationsRequested, this,
            &MainWindow::onMuteNotifications);
    connect(m_tray_icon, &SystemTrayIcon::addTaskRequested, this,
            &MainWindow::onAddTask);
    connect(m_tray_icon, &SystemTrayIcon::exitRequested, this,
            &MainWindow::quitApp);

    connect(m_tray_icon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger) {
                    this->toggleMainWindow();
                }
            });

    m_tray_icon->show();
}

void MainWindow::initFileMenu()
{
    auto *file_menu = menuBar()->addMenu(tr("&File"));
    file_menu->setToolTipsVisible(true);

    auto *settings = new QAction("&Settings", this);
    settings->setShortcut(kSettingsButtonShortcut);
    file_menu->addAction(settings);
    connect(settings, &QAction::triggered, this, &MainWindow::onOpenSettings);

    auto *quit = new QAction("&Quit", this);
    quit->setShortcut(QKeySequence::Quit);
    file_menu->addAction(quit);
    connect(quit, &QAction::triggered, this, &MainWindow::quitApp);
}

void MainWindow::connectViewMenuActions()
{
    connect(m_view_menu_actions.m_toggle_task_shell_action, &QAction::triggered,
            this, &MainWindow::onToggleTaskShell);
}

void MainWindow::initToolsMenu()
{
    auto *tools_menu = menuBar()->addMenu(tr("&Tools"));
    tools_menu->setToolTipsVisible(true);

    auto *agenda_action = new QAction("&Agenda view", this);
    tools_menu->addAction(agenda_action);
    agenda_action->setShortcut(kAgendaViewShortcut);
    connect(agenda_action, &QAction::triggered, this, [&]() {
        if (auto tasks = m_task_provider->getTasks()) {
            auto *dlg = new AgendaDialog(std::move(*tasks), this);
            dlg->open();
            QObject::connect(dlg, &QDialog::finished, dlg,
                             &QDialog::deleteLater);
        }
    });

    auto *recurring_action = new QAction("&Recurring templates", this);
    tools_menu->addAction(recurring_action);
    recurring_action->setShortcut(kRecurrentViewShortcut);
    connect(recurring_action, &QAction::triggered, this, [&]() {
        if (auto tasks = m_task_provider->getRecurringTasks()) {
            auto *dlg = new RecurringDialog(std::move(*tasks), this);
            dlg->open();
            QObject::connect(dlg, &QDialog::finished, dlg,
                             &QDialog::deleteLater);
        }
    });

    auto *history_stats_action = new QAction("&Statistics", this);
    history_stats_action->setEnabled(false);
    tools_menu->addAction(history_stats_action);
    connect(history_stats_action, &QAction::triggered, this,
            &MainWindow::onToggleTaskShell);

    tools_menu->setVisible(false);
}

void MainWindow::initHelpMenu()
{
    QMenu *help_menu = menuBar()->addMenu(tr("&Help"));
    help_menu->setToolTipsVisible(true);

    auto *reference_action = new QAction("&Taskwarrior quick reference", this);
    help_menu->addAction(reference_action);
    connect(reference_action, &QAction::triggered, this, [&]() {
        auto *dlg = new TaskwarriorReferenceDialog(this);
        dlg->open();
        QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);
    });

    help_menu->addSeparator();

    auto *about_action = new QAction("&About", this);
    help_menu->addAction(about_action);
    about_action->setShortcut(tr("F1"));
    connect(about_action, &QAction::triggered, this, [&]() {
        auto *dlg = new AboutDialog(m_task_provider->getTaskVersion(), this);
        dlg->open();
        QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);
    });
}

void MainWindow::initShortcuts()
{
    auto *focus_task_shell = new QAction(this);
    focus_task_shell->setShortcut(kActivateTaskShellShortcut);
    connect(focus_task_shell, &QAction::triggered, this, [&]() {
        if (m_task_shell->isVisible()) {
            m_task_shell->setFocus();
        }
    });
    addAction(focus_task_shell);

    auto *filter_tasks = new QAction(this);
    filter_tasks->setShortcut(QKeySequence::Find);
    connect(filter_tasks, &QAction::triggered, this,
            [&]() { m_task_filter->setFocus(); });
    addAction(filter_tasks);
}

void MainWindow::connectTaskToolbarActions()
{
    connect(m_toolbar_actions.m_add_action, &QAction::triggered, this,
            [&]() { onAddTask(); });

    connect(m_toolbar_actions.m_undo_action, &QAction::triggered, this, [&]() {
        m_task_provider->undoTask();
        m_toolbar_actions.m_undo_action->setEnabled(
            m_task_provider->getActionsCounter() > 0);
        refreshTasksListTableIfNeeded();
    });

    connect(m_toolbar_actions.m_refresh_action, &QAction::triggered, this,
            [&]() { refreshTasksListTableEnforced(); });

    connect(m_toolbar_actions.m_done_action, &QAction::triggered, this,
            &MainWindow::onSetTasksDone);

    connect(m_toolbar_actions.m_edit_action, &QAction::triggered, this,
            &MainWindow::onEditTaskAction);

    connect(m_toolbar_actions.m_wait_action, &QAction::triggered, this, [&]() {
        auto *dlg =
            new DateTimeDialog(QDateTime::currentDateTime().addDays(1), this);
        dlg->open();
        QObject::connect(dlg, &QDialog::accepted, [this, dlg]() {
            if (m_task_provider->waitTask(getSelectedTaskIds(),
                                          dlg->getDateTime())) {
                refreshTasksListTableIfNeeded();
            }
        });
        QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);
    });

    connect(m_toolbar_actions.m_delete_action, &QAction::triggered, this,
            &MainWindow::onDeleteTasks);

    connect(m_toolbar_actions.m_start_action, &QAction::triggered, this, [&]() {
        if (auto t_opt = getSelectedTaskId()) {
            m_task_provider->startTask(*t_opt);
            refreshTasksListTableIfNeeded();
        }
    });

    connect(m_toolbar_actions.m_stop_action, &QAction::triggered, this, [&]() {
        if (auto t_opt = getSelectedTaskId()) {
            m_task_provider->stopTask(*t_opt);
            refreshTasksListTableIfNeeded();
        }
    });
}

void MainWindow::toggleMainWindow()
{
    if (this->isHidden()) {
        showMainWindow();
    } else {
        this->hide();
    }
}

void MainWindow::onOpenSettings()
{
    auto *dlg = new SettingsDialog(this);
    dlg->open();
    QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);
}

void MainWindow::showMainWindow()
{
    if (this->isMinimized()) {
        if (m_window_prev_state & Qt::WindowMaximized) {
            this->showMaximized();
        } else if (m_window_prev_state & Qt::WindowFullScreen) {
            this->showFullScreen();
        } else {
            this->showNormal();
        }
    } else {
        this->show();
        this->raise();
    }

    this->activateWindow();
}

void MainWindow::receiveNewInstanceMessage(quint32, const QByteArray &message)
{
    if (message == "add_task") {
        onAddTask();
    } else {
        showMainWindow();
    }
}

void MainWindow::quitApp()
{
    if (ConfigManager::config().getSaveFilterOnExit()) {
        ConfigManager::config().setTaskFilter(m_task_filter->getTags());
    }

    m_is_quit = true;
    close();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    auto set_task_priority = [&](DetailedTaskInfo::Priority p) {
        const auto *smodel = m_tasks_view->selectionModel();
        if (!smodel->hasSelection()) {
            return;
        }
        const auto *dmodel = qobject_cast<TasksModel *>(m_tasks_view->model());
        for (const QModelIndex idx : smodel->selectedRows()) {
            auto item = dmodel->itemData(idx);
            if (!item[0].isNull()) {
                m_task_provider->setPriority(item[0].toString(), p);
                break;
            }
        }
    };

    if (watched == m_tasks_view && event->type() == QEvent::KeyPress) {
        const auto keyEvent = dynamic_cast<QKeyEvent *>(event);
        assert(keyEvent);

        switch (keyEvent->key()) {
        case Qt::Key_Escape:
            m_tasks_view->selectionModel()->clearSelection();
            return true;
        default: {
            static const std::map<int, DetailedTaskInfo::Priority> key2priority{
                { Qt::Key_0, DetailedTaskInfo::Priority::Unset },
                { Qt::Key_3, DetailedTaskInfo::Priority::L },
                { Qt::Key_2, DetailedTaskInfo::Priority::M },
                { Qt::Key_1, DetailedTaskInfo::Priority::H },
            };
            const auto it = key2priority.find(keyEvent->key());
            if (it == key2priority.end()) {
                break;
            }
            set_task_priority(it->second);
            return true;
        }
        }
    }

    // Let any other handlers do their thing
    return false;
}

void MainWindow::changeEvent(QEvent *evt)
{
    if (evt->type() == QEvent::WindowStateChange) {
        const auto e = dynamic_cast<QWindowStateChangeEvent *>(evt);
        assert(e);
        m_window_prev_state = e->oldState();
    }

    QMainWindow::changeEvent(evt);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_is_quit && m_tray_icon->isVisible()) {
        hide();
        event->ignore();
    } else {
        ConfigManager::config().updateConfigFile();
        QMainWindow::closeEvent(event);
        qApp->quit();
    }
}

std::optional<QString> MainWindow::getSelectedTaskId() const
{
    Q_ASSERT(m_tasks_view);

    auto *smodel = m_tasks_view->selectionModel();
    if (smodel->selectedRows().size() != 1) {
        return {};
    }

    auto *dmodel = qobject_cast<TasksModel *>(m_tasks_view->model());
    for (const QModelIndex idx : smodel->selectedRows()) {
        auto item = dmodel->itemData(idx);
        if (item[0].isNull()) {
            continue;
        }
        return item[0].toString();
    }

    return {};
}

QStringList MainWindow::getSelectedTaskIds() const
{
    auto *smodel = m_tasks_view->selectionModel();
    if (!smodel->hasSelection()) {
        return {};
    }

    QStringList ids;

    auto *dmodel = qobject_cast<TasksModel *>(m_tasks_view->model());
    for (const QModelIndex idx : smodel->selectedRows()) {
        auto item = dmodel->itemData(idx);
        if (item[0].isNull()) {
            continue;
        }
        ids.push_back(item[0].toString());
    }

    return ids;
}

void MainWindow::pushFilterTag(const QString &value)
{
    if (!m_task_filter->isVisible()) {
        return;
    }
    m_task_filter->pushTag(value);
}

void MainWindow::onToggleTaskShell()
{
    if (m_view_menu_actions.m_toggle_task_shell_action->isChecked()) {
        m_task_shell->setVisible(true);
        ConfigManager::config().setShowTaskShell(true);
    } else {
        m_task_shell->setVisible(false);
        ConfigManager::config().setShowTaskShell(false);
    }
}

void MainWindow::onSettingsMenu() {}

void MainWindow::onMuteNotifications() {}

void MainWindow::onAddTask()
{
    QVariant default_project = {};
    for (const auto &tag : m_task_filter->getTags()) {
        if (tag.startsWith("pro:") || tag.startsWith("project:")) {
            if (!default_project.isNull()) {
                default_project = {};
                break;
            }
            default_project = { tag.mid(tag.indexOf(':') + 1, tag.size()) };
        }
    }

    auto dlg =
        QPointer<AddTaskDialog>(new AddTaskDialog(default_project, this));
    dlg->open();

    QObject::connect(this, &MainWindow::acceptContinueCreatingTasks, dlg,
                     &AddTaskDialog::acceptContinue);
    QObject::connect(dlg, &QDialog::accepted, [this, dlg]() {
        Q_ASSERT(dlg);
        auto t = dlg->getTask();
        if (m_task_provider->addTask(t)) {
            refreshTasksListTableIfNeeded();
        }
    });
    QObject::connect(dlg, &QDialog::rejected,
                     [this]() { refreshTasksListTableIfNeeded(); });
    QObject::connect(dlg, &AddTaskDialog::createTaskAndContinue, [this, dlg]() {
        Q_ASSERT(dlg);
        auto t = dlg->getTask();
        if (m_task_provider->addTask(t)) {
            refreshTasksListTableIfNeeded();
            emit acceptContinueCreatingTasks();
        } else {
            dlg->close();
        }
    });
    QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);
}

void MainWindow::onDeleteTasks()
{
    const auto selectedTasks = getSelectedTaskIds();
    const auto selectedCount = selectedTasks.size();
    if (selectedCount == 0) {
        return;
    }
    const auto question_to_show =
        (selectedCount == 1) ? tr("Delete task?")
                             : tr("Delete %1 tasks?").arg(selectedCount);

    if (QMessageBox::question(this, tr("Conifrm action"), question_to_show,
                              QMessageBox::Yes | QMessageBox::No) ==
        QMessageBox::Yes) {
        m_tasks_view->selectionModel()->clearSelection();
        m_task_provider->deleteTask(selectedTasks);
        refreshTasksListTableIfNeeded();
    }
}

void MainWindow::onSetTasksDone()
{
    if (!m_tasks_view->selectionModel()->hasSelection()) {
        return;
    }
    m_task_provider->setTaskDone(getSelectedTaskIds());
    m_tasks_view->selectionModel()->clearSelection();
    refreshTasksListTableIfNeeded();
}

void MainWindow::onApplyFilter()
{
    if (m_task_provider->applyFilter(m_task_filter->getTags())) {
        refreshTasksListTableEnforced();
        return;
    }
    refreshTasksListTableIfNeeded();
}

void MainWindow::onEnterTaskCommand()
{
    auto cmd = m_task_shell->text();
    if (cmd.isEmpty()) {
        return;
    }
    auto rc = m_task_provider->directCmd(cmd);
    if (rc == 0) {
        refreshTasksListTableIfNeeded();
    }
    m_task_shell->setText("");
}

void MainWindow::showEditTaskDialog([[maybe_unused]] const QModelIndex &idx)
{
    const auto *model = m_tasks_view->selectionModel();
    const auto id_str = model->selectedRows()[0].data().toString();
    if (id_str.isEmpty()) {
        refreshTasksListTableIfNeeded();
        return;
    }

    const auto task = m_task_provider->getTask(id_str);
    if (!task) {
        refreshTasksListTableIfNeeded();
        return;
    }

    auto dlg = QPointer<EditTaskDialog>(new EditTaskDialog(*task, this));
    QObject::connect(dlg, &EditTaskDialog::deleteTask, this,
                     [&](const QString &uuid) {
                         m_task_provider->deleteTask(uuid);
                         m_tasks_view->selectionModel()->clearSelection();
                         refreshTasksListTableIfNeeded();
                     });
    QObject::connect(dlg, &QDialog::accepted, [this, dlg, id_str]() {
        Q_ASSERT(dlg);
        auto tmp = dlg->getTask();
        m_task_provider->editTask(tmp);
        refreshTasksListTableIfNeeded();
    });
    QObject::connect(dlg, &QDialog::rejected,
                     [this]() { refreshTasksListTableIfNeeded(); });
    QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);

    dlg->open();
}

void MainWindow::onEditTaskAction()
{
    const auto *model = m_tasks_view->selectionModel();
    Q_ASSERT(model->selectedRows().size() == 1);
    showEditTaskDialog(model->selectedRows()[0]);
}

void MainWindow::refreshTasksListTableIfNeeded()
{
    // It may NOT refresh if not needed.
    m_task_watcher->checkNow();
}

void ui::MainWindow::refreshTasksListTableEnforced()
{
    // It WILL refresh unconditionally (with current filters).
    m_task_watcher->enforceUpdate();
    refreshTasksListTableIfNeeded();
}

void MainWindow::updateTaskToolbar()
{
    const auto *model = m_tasks_view->selectionModel();
    const auto num_selected = model->selectedRows().size();

    m_toolbar_actions.m_undo_action->setEnabled(
        m_task_provider->getActionsCounter() > 0);

    if (num_selected == 0u) {
        m_toolbar_actions.m_done_action->setEnabled(false);
        m_toolbar_actions.m_edit_action->setEnabled(false);
        m_toolbar_actions.m_wait_action->setEnabled(false);
        m_toolbar_actions.m_delete_action->setEnabled(false);
        m_toolbar_actions.m_start_action->setEnabled(false);
        m_toolbar_actions.m_stop_action->setEnabled(false);
        removeShortcutFromToolTip(m_toolbar_actions.m_stop_action);
        removeShortcutFromToolTip(m_toolbar_actions.m_start_action);
    } else {
        m_toolbar_actions.m_done_action->setEnabled(true);
        if (num_selected == 1) {
            m_toolbar_actions.m_edit_action->setEnabled(true);
        } else {
            m_toolbar_actions.m_edit_action->setEnabled(false);
        }
        m_toolbar_actions.m_wait_action->setEnabled(true);
        m_toolbar_actions.m_delete_action->setEnabled(true);
    }
}

MainWindow::TToolbarActionsDefined::TToolbarActionsDefined(QToolBar &parent)
{
    const auto allocate = [&parent](const QString &path, const QString &text,
                                    const QKeySequence &short_cut) {
        auto action = new QAction(QIcon(path), text, &parent);
        action->setEnabled(true);
        action->setShortcut(short_cut);
        removeShortcutFromToolTip(action);
        addShortcutToToolTip(action);
        parent.addAction(action);
        return action;
    };

    m_add_action =
        allocate(":/icons/add.svg", tr("Add task"), QKeySequence::New);
    m_undo_action = allocate(":/icons/undo.png", tr("Undo last action"),
                             QKeySequence::Undo);
    m_undo_action->setEnabled(false);

    m_refresh_action = allocate(":/icons/refresh.png", tr("Refresh tasks"),
                                QKeySequence::Refresh);

    parent.addSeparator();

    m_done_action = allocate(":/icons/done.svg", tr("Done"), kDoneShortcut);
    m_done_action->setEnabled(false);

    m_edit_action = allocate(":/icons/edit.svg", tr("Edit"), kEditShortcut);
    m_edit_action->setEnabled(false);

    m_wait_action = allocate(":/icons/wait.svg", tr("Wait"), kWaitShortcut);
    m_wait_action->setEnabled(false);

    m_delete_action =
        allocate(":/icons/delete.svg", tr("Delete"), QKeySequence::Delete);
    m_delete_action->setEnabled(false);

    parent.addSeparator();

    m_start_action =
        allocate(":/icons/start.svg", tr("Start"), QKeySequence::UnknownKey);
    m_start_action->setEnabled(false);

    m_stop_action =
        allocate(":/icons/stop.svg", tr("Stop"), QKeySequence::UnknownKey);
    m_stop_action->setEnabled(false);
}

MainWindow::TViewMenuActions::TViewMenuActions(QMenu &parent)
{
    parent.setToolTipsVisible(true);
    m_toggle_task_shell_action = new QAction(tr("&Task shell"), &parent);
    m_toggle_task_shell_action->setCheckable(true);
    m_toggle_task_shell_action->setChecked(
        ConfigManager::config().getShowTaskShell());
    parent.addAction(m_toggle_task_shell_action);
}
