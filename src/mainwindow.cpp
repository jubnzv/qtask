#include "mainwindow.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QDialog>
#include <QEvent>
#include <QGridLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QTableView>
#include <QToolBar>
#include <QVariant>
#include <QWindowStateChangeEvent>

#include "aboutdialog.hpp"
#include "configmanager.hpp"
#include "datetimedialog.hpp"
#include "qtutil.hpp"
#include "settingsdialog.hpp"
#include "tagsedit.hpp"
#include "taskdialog.hpp"
#include "tasksmodel.hpp"
#include "tasksview.hpp"
#include "taskwarrior.hpp"
#include "trayicon.hpp"

using namespace ui;

MainWindow::MainWindow()
    : m_window_prev_state(Qt::WindowNoState)
    , m_is_quit(false)
    , m_task_provider(std::make_unique<Taskwarrior>())
    , m_task_watcher(nullptr)
{
    if (!initTaskWatcher()) {
        QMessageBox::warning(
            this, tr("Error"),
            tr("Can't initialize file watcher service for %1. The task "
               "list will not be updated after external changes.")
                .arg(ConfigManager::config()->getTaskDataPath()));
    }

    initMainWindow();
    initTrayIcon();
    initFileMenu();
    initViewMenu();
    initToolsMenu();
    initHelpMenu();
    initShortcuts();
}

MainWindow::~MainWindow()
{
    if (m_task_watcher) {
        delete m_task_watcher;
        m_task_watcher = nullptr;
    }
}

bool MainWindow::initTaskWatcher()
{
    Q_ASSERT(!m_task_watcher);
    m_task_watcher = new TaskWatcher();
    if (!m_task_watcher->setup(ConfigManager::config()->getTaskDataPath())) {
        delete m_task_watcher;
        m_task_watcher = nullptr;
        return false;
    }
    connect(
        m_task_watcher, &TaskWatcher::dataChanged, this,
        [&](const QString & /* filepath */) { updateTasks(/*force=*/true); });
    return true;
}

void MainWindow::initMainWindow()
{
    setWindowIcon(QIcon(":/icons/jtask.svg"));
    setWindowTitle(QCoreApplication::applicationName());
    setMinimumSize(400, 500);

    m_window = new QWidget();

    m_layout = new QGridLayout();

    initTaskToolbar();

    initTasksTable();

    m_task_shell = new QLineEdit();
    m_task_shell->addAction(QIcon(":/icons/taskwarrior.png"),
                            QLineEdit::LeadingPosition);
    connect(m_task_shell, &QLineEdit::returnPressed, this,
            &MainWindow::onEnterTaskCommand);

    m_task_filter = new TagsEdit(/* TODO: QIcon(":/icons/filter.svg")*/);
    connect(m_task_filter, &TagsEdit::tagsChanged, this,
            &MainWindow::onApplyFilter);

    m_layout->addWidget(m_task_toolbar, 0, 0);
    m_layout->addWidget(m_task_filter, 0, 1);
    m_layout->addWidget(m_tasks_view, 1, 0, 1, 2);
    m_layout->addWidget(m_task_shell, 2, 0, 1, 2);

    m_window->setLayout(m_layout);
    setCentralWidget(m_window);

    updateTasks(/*force=*/true);
}

void MainWindow::initTasksTable()
{
    m_tasks_view = new TasksView(m_window);
    m_tasks_view->setShowGrid(true);

    m_tasks_view->verticalHeader()->setVisible(false);
    m_tasks_view->horizontalHeader()->setStretchLastSection(true);
    m_tasks_view->setSelectionBehavior(QAbstractItemView::SelectRows);

    TasksModel *model = new TasksModel();
    m_tasks_view->setModel(model);

    connect(m_tasks_view->selectionModel(),
            &QItemSelectionModel::selectionChanged, this,
            &MainWindow::updateTaskToolbar);
    connect(m_tasks_view, &TasksView::pushProjectFilter, this,
            &MainWindow::pushFilterTag);
    connect(m_tasks_view, &QTableView::doubleClicked, this,
            &MainWindow::showEditTaskDialog);

    m_tasks_view->installEventFilter(this);
}

void MainWindow::initTrayIcon()
{
    m_tray_icon = new SystemTrayIcon(this);

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
    QMenu *file_menu = menuBar()->addMenu(tr("&File"));
    file_menu->setToolTipsVisible(true);

    QAction *settings = new QAction("&Settings", this);
    settings->setShortcut(tr("CTRL+SHIFT+P"));
    file_menu->addAction(settings);
    connect(settings, &QAction::triggered, this,
            &MainWindow::openSettingsDialog);

    QAction *quit = new QAction("&Quit", this);
    quit->setShortcut(tr("CTRL+Q"));
    file_menu->addAction(quit);
    connect(quit, &QAction::triggered, this, &MainWindow::quitApp);
}

void MainWindow::initViewMenu()
{
    QMenu *view_menu = menuBar()->addMenu(tr("&View"));
    view_menu->setToolTipsVisible(true);

    m_toggle_task_shell_action = new QAction("&Task shell", this);
    m_toggle_task_shell_action->setCheckable(true);
    m_toggle_task_shell_action->setChecked(true);
    view_menu->addAction(m_toggle_task_shell_action);
    connect(m_toggle_task_shell_action, &QAction::triggered, this,
            &MainWindow::onToggleTaskShell);

    m_toggle_task_filter_action = new QAction("&Task filter", this);
    m_toggle_task_filter_action->setCheckable(true);
    m_toggle_task_filter_action->setChecked(true);
    view_menu->addAction(m_toggle_task_filter_action);
    connect(m_toggle_task_filter_action, &QAction::triggered, this,
            &MainWindow::onToggleTaskFilter);
}

void MainWindow::initToolsMenu()
{
    QMenu *tools_menu = menuBar()->addMenu(tr("&Tools"));
    tools_menu->setToolTipsVisible(true);

    QAction *history_stats_action = new QAction("&Statistics", this);
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

    QAction *about_action = new QAction("&About", this);
    help_menu->addAction(about_action);
    about_action->setShortcut(tr("F1"));
    connect(about_action, &QAction::triggered, this, [&]() {
        auto *dlg = new AboutDialog(this);
        dlg->open();
        QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);
    });
}

void MainWindow::initShortcuts()
{
    QAction *focus_task_shell = new QAction(this);
    focus_task_shell->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_T));
    connect(focus_task_shell, &QAction::triggered, this,
            &MainWindow::onFocusTaskShell);
    this->addAction(focus_task_shell);

    QAction *filter_tasks = new QAction(this);
    filter_tasks->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F));
    connect(filter_tasks, &QAction::triggered, this,
            &MainWindow::onFocusTaskFilter);
    this->addAction(filter_tasks);
}

void MainWindow::initTaskToolbar()
{
    m_task_toolbar = new QToolBar("Task toolbar");

    m_add_action = new QAction(QIcon(":/icons/add.svg"), tr("Add task"));
    connect(m_add_action, &QAction::triggered, this, [&]() {
        m_tasks_view->selectionModel()->clearSelection();
        onAddTask();
    });
    m_add_action->setShortcut(tr("CTRL+N"));
    removeShortcutFromToolTip(m_add_action);
    addShortcutToToolTip(m_add_action);
    m_task_toolbar->addAction(m_add_action);

    m_undo_action =
        new QAction(QIcon(":/icons/undo.png"), tr("Undo last action"));
    connect(m_undo_action, &QAction::triggered, this, [&]() {
        if (m_task_provider->undoTask())
            m_tasks_view->selectionModel()->clearSelection();
        m_undo_action->setEnabled(m_task_provider->getActionsCounter() > 0);
    });
    m_undo_action->setEnabled(false);
    m_undo_action->setShortcut(tr("CTRL+Z"));
    removeShortcutFromToolTip(m_undo_action);
    addShortcutToToolTip(m_undo_action);
    m_task_toolbar->addAction(m_undo_action);

    m_update_action =
        new QAction(QIcon(":/icons/refresh.png"), tr("Update tasks"));
    connect(m_update_action, &QAction::triggered, this, [&]() {
        m_tasks_view->selectionModel()->clearSelection();
        updateTasks(/*force=*/true);
    });
    m_update_action->setShortcut(tr("CTRL+R"));
    removeShortcutFromToolTip(m_update_action);
    addShortcutToToolTip(m_update_action);
    m_task_toolbar->addAction(m_update_action);

    m_task_toolbar->addSeparator();

    m_done_action = new QAction(QIcon(":/icons/done.svg"), tr("Done"));
    connect(m_done_action, &QAction::triggered, this,
            &MainWindow::onSetTasksDone);
    m_done_action->setShortcut(tr("CTRL+D"));
    removeShortcutFromToolTip(m_done_action);
    addShortcutToToolTip(m_done_action);
    m_task_toolbar->addAction(m_done_action);
    m_task_toolbar->addAction(m_done_action);
    m_done_action->setEnabled(false);

    m_edit_action = new QAction(QIcon(":/icons/edit.svg"), tr("Edit"));
    connect(m_edit_action, &QAction::triggered, this,
            &MainWindow::onEditTaskAction);
    m_edit_action->setShortcut(tr("CTRL+E"));
    removeShortcutFromToolTip(m_edit_action);
    addShortcutToToolTip(m_edit_action);
    m_task_toolbar->addAction(m_edit_action);
    m_edit_action->setEnabled(false);

    m_wait_action = new QAction(QIcon(":/icons/wait.svg"), tr("Wait"));
    connect(m_wait_action, &QAction::triggered, this, [&]() {
        auto *dlg =
            new DateTimeDialog(QDateTime::currentDateTime().addDays(1), this);
        dlg->open();
        QObject::connect(dlg, &QDialog::accepted, [this, dlg]() {
            auto dt = dlg->getDateTime();
            if (m_task_provider->waitTask(getSelectedTaskIds(), dt))
                updateTasks();
        });
        QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);
    });
    m_wait_action->setShortcut(tr("CTRL+W"));
    removeShortcutFromToolTip(m_wait_action);
    addShortcutToToolTip(m_wait_action);
    m_task_toolbar->addAction(m_wait_action);
    m_wait_action->setEnabled(false);

    m_delete_action = new QAction(QIcon(":/icons/delete.svg"), tr("Delete"));
    connect(m_delete_action, &QAction::triggered, this,
            &MainWindow::onDeleteTasks);
    m_delete_action->setShortcut(tr("Delete"));
    removeShortcutFromToolTip(m_delete_action);
    addShortcutToToolTip(m_delete_action);
    m_task_toolbar->addAction(m_delete_action);
    m_delete_action->setEnabled(false);

    m_task_toolbar->addSeparator();

    m_start_action = new QAction(QIcon(":/icons/start.svg"), tr("Start"));
    connect(m_start_action, &QAction::triggered, this, [&]() {
        auto t_opt = getSelectedTaskId();
        if (t_opt.isNull())
            return;
        m_task_provider->startTask(t_opt.toString());
        updateTasks();
    });
    m_start_action->setShortcut(tr("CTRL+S"));
    removeShortcutFromToolTip(m_start_action);
    addShortcutToToolTip(m_start_action);
    m_task_toolbar->addAction(m_start_action);
    m_start_action->setEnabled(false);

    m_stop_action = new QAction(QIcon(":/icons/stop.svg"), tr("Stop"));
    connect(m_stop_action, &QAction::triggered, this, [&]() {
        auto t_opt = getSelectedTaskId();
        if (t_opt.isNull())
            return;
        m_task_provider->stopTask(t_opt.toString());
        updateTasks();
    });
    m_stop_action->setShortcut(tr("CTRL+ALT+S"));
    removeShortcutFromToolTip(m_stop_action);
    addShortcutToToolTip(m_stop_action);
    m_task_toolbar->addAction(m_stop_action);
    m_stop_action->setEnabled(false);
}

void MainWindow::toggleMainWindow()
{
    if (this->isHidden()) {
        showMainWindow();
    } else {
        this->hide();
    }
}

void MainWindow::openSettingsDialog()
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

void MainWindow::quitApp()
{
    m_is_quit = true;
    close();
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    auto set_task_priority = [&](Task::Priority p) {
        auto *smodel = m_tasks_view->selectionModel();
        if (!smodel->hasSelection())
            return;
        auto *dmodel = qobject_cast<TasksModel *>(m_tasks_view->model());
        for (const QModelIndex idx : smodel->selectedRows()) {
            auto item = dmodel->itemData(idx);
            if (item[0].isNull())
                continue;
            QString tid_str = item[0].toString();
            m_task_provider->setPriority(tid_str, p);
        }
    };

    if (watched == m_tasks_view && event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            m_tasks_view->selectionModel()->clearSelection();
            return true;
        }
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_0) {
            set_task_priority(Task::Priority::Unset);
            return true;
        }
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_3) {
            set_task_priority(Task::Priority::L);
            return true;
        }
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_2) {
            set_task_priority(Task::Priority::M);
            return true;
        }
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_1) {
            set_task_priority(Task::Priority::H);
            return true;
        }
    }

    // Let any other handlers do their thing
    return false;
}

void MainWindow::changeEvent(QEvent *evt)
{
    if (evt->type() == QEvent::WindowStateChange) {
        QWindowStateChangeEvent *e =
            dynamic_cast<QWindowStateChangeEvent *>(evt);
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
        QMainWindow::closeEvent(event);
    }
}

QVariant MainWindow::getSelectedTaskId()
{
    Q_ASSERT(m_tasks_view);

    auto *smodel = m_tasks_view->selectionModel();
    if (smodel->selectedRows().size() != 1)
        return {};

    auto *dmodel = qobject_cast<TasksModel *>(m_tasks_view->model());
    for (const QModelIndex idx : smodel->selectedRows()) {
        auto item = dmodel->itemData(idx);
        if (item[0].isNull())
            continue;
        return item[0].toString();
    }

    return {};
}

QStringList MainWindow::getSelectedTaskIds()
{
    auto *smodel = m_tasks_view->selectionModel();
    if (!smodel->hasSelection())
        return {};

    QStringList ids;

    auto *dmodel = qobject_cast<TasksModel *>(m_tasks_view->model());
    for (const QModelIndex idx : smodel->selectedRows()) {
        auto item = dmodel->itemData(idx);
        if (item[0].isNull())
            continue;
        ids.push_back(item[0].toString());
    }

    return ids;
}

void MainWindow::pushFilterTag(const QString &value)
{
    if (!m_task_filter->isVisible())
        return;
    m_task_filter->pushTag(value);
}

void MainWindow::onToggleTaskShell()
{
    if (m_toggle_task_shell_action->isChecked()) {
        m_task_shell->setVisible(true);
    } else {
        m_task_shell->setVisible(false);
    }
}

void MainWindow::onToggleTaskFilter()
{
    if (m_toggle_task_filter_action->isChecked()) {
        m_task_filter->setVisible(true);
    } else {
        m_task_filter->setVisible(false);
    }
}

void MainWindow::onFocusTaskShell()
{
    if (!m_task_shell->isVisible()) {
        m_task_shell->setVisible(true);
    }
    m_task_shell->setFocus();
}

void MainWindow::onFocusTaskFilter()
{
    if (!m_task_filter->isVisible()) {
        m_task_filter->setVisible(true);
    }
    m_task_filter->setFocus();
}

void MainWindow::onSettingsMenu() {}

void MainWindow::onMuteNotifications() {}

void MainWindow::onAddTask()
{
    auto *dlg = new TaskDialog(this);
    dlg->open();
    QObject::connect(dlg, &QDialog::accepted, [this, dlg]() {
        auto t = dlg->getTask();
        if (m_task_provider->addTask(t))
            updateTasks();
    });

    QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);
}

void MainWindow::onDeleteTasks()
{
    auto *smodel = m_tasks_view->selectionModel();
    QString q;
    if (smodel->selectedRows().size() == 1)
        q = tr("Delete task?");
    else
        q = tr("Delete %1 tasks?").arg(smodel->selectedRows().size());
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Conifrm action"), q,
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_task_provider->deleteTask(getSelectedTaskIds());
        smodel->clearSelection();
        updateTasks();
    }
}

void MainWindow::onSetTasksDone()
{
    if (!m_tasks_view->selectionModel()->hasSelection())
        return;
    m_task_provider->setTaskDone(getSelectedTaskIds());
    m_tasks_view->selectionModel()->clearSelection();
    updateTasks();
}

void MainWindow::onApplyFilter()
{
    if (!m_task_provider->applyFilter(m_task_filter->getTags()))
        m_task_filter->popTag();
    // if (!m_task_provider->applyFilter(m_task_filter->getTags()))
    //     m_task_filter->clearTags();
    updateTasks(/*force=*/true);
}

void MainWindow::onEnterTaskCommand()
{
    auto cmd = m_task_shell->text();
    if (cmd.isEmpty())
        return;
    auto rc = m_task_provider->directCmd(cmd);
    if (rc == 0) {
        updateTasks();
    }
    m_task_shell->setText("");
}

void MainWindow::showEditTaskDialog(const QModelIndex &idx)
{
    const auto *model = m_tasks_view->selectionModel();

    QString id_str = model->selectedRows()[0].data().toString();
    if (id_str.isEmpty()) {
        updateTasks();
        return;
    }

    Task task;
    if (!m_task_provider->getTask(id_str, task)) {
        updateTasks();
        return;
    }

    auto *dlg = new TaskDialog(task, this);

    dlg->open();
    QObject::connect(dlg, &QDialog::accepted, [this, dlg, id_str, task]() {
        auto saved_tags = task.tags;
        auto saved_pri = task.priority;
        auto t = dlg->getTask();
        auto new_tags = t.tags;
        t.removed_tags = QStringList();
        for (auto const &st : saved_tags)
            if (!new_tags.contains(st))
                t.removed_tags.push_back(st);
        t.uuid = id_str;
        if (!m_task_provider->editTask(t))
            return;
        if (saved_pri != t.priority &&
            !m_task_provider->setPriority(t.uuid, t.priority))
            return;
        updateTasks();
    });

    QObject::connect(dlg, &QDialog::finished, dlg, &QDialog::deleteLater);
}

void MainWindow::onEditTaskAction()
{
    const auto *model = m_tasks_view->selectionModel();
    Q_ASSERT(model->selectedRows().size() == 1);
    showEditTaskDialog(model->selectedRows()[0]);
}

void MainWindow::updateTasks(bool force)
{
    Q_ASSERT(m_task_provider);

    // The commands from m_task_provider will modify the taskwarrior
    // database files. This will trigger TaskWatcher to emit update event.
    if (!force && m_task_watcher->isActive())
        return;

    QList<Task> tasks = {};
    m_task_provider->getTasks(tasks);

    auto *model = qobject_cast<TasksModel *>(m_tasks_view->model());
    model->setTasks(std::move(tasks));
}

void MainWindow::updateTaskToolbar()
{
    const auto *model = m_tasks_view->selectionModel();
    const int num_selected = model->selectedRows().size();

    m_undo_action->setEnabled(m_task_provider->getActionsCounter() > 0);

    if (num_selected == 0) {
        m_done_action->setEnabled(false);
        m_edit_action->setEnabled(false);
        m_wait_action->setEnabled(false);
        m_delete_action->setEnabled(false);
        m_start_action->setEnabled(false);
        m_stop_action->setEnabled(false);
    } else {
        m_done_action->setEnabled(true);
        if (num_selected == 1) {
            m_edit_action->setEnabled(true);
            m_start_action->setEnabled(true);
            m_stop_action->setEnabled(true);
        } else {
            m_edit_action->setEnabled(false);
            m_start_action->setEnabled(false);
            m_stop_action->setEnabled(false);
        }
        m_wait_action->setEnabled(true);
        m_delete_action->setEnabled(true);
    }
}
