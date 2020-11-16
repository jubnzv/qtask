#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <memory>

#include <QCloseEvent>
#include <QEvent>
#include <QGridLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QTableView>
#include <QVariant>

#include "tagsedit.hpp"
#include "task.hpp"
#include "tasksview.hpp"
#include "taskwarrior.hpp"
#include "taskwatcher.hpp"
#include "trayicon.hpp"

namespace ui
{
class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow();
    ~MainWindow();

  private:
    bool initTaskWatcher();

    void initMainWindow();
    void initTasksTable();
    void initTrayIcon();
    void initFileMenu();
    void initViewMenu();
    void initToolsMenu();
    void initHelpMenu();
    void initShortcuts();
    void initTaskToolbar();
    void toggleMainWindow();
    void openSettingsDialog();
    void showMainWindow();
    void quitApp();

    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *) override;
    void closeEvent(QCloseEvent *event) override;

    QVariant getSelectedTaskId();
    QStringList getSelectedTaskIds();

  public slots:
    /// Add entry to tags filter
    void pushFilterTag(const QString &);

  private slots:
    void onToggleTaskShell();
    void onToggleTaskFilter();
    void onFocusTaskShell();
    void onFocusTaskFilter();
    void onSettingsMenu();
    void onMuteNotifications();
    void onAddTask();
    void onDeleteTasks();
    void onSetTasksDone();
    void onEnterTaskCommand();
    void onApplyFilter();
    void onEditTaskAction();
    void showEditTaskDialog(const QModelIndex &);

    void updateTasks(bool force = false);
    void updateTaskToolbar();

  private:
    /// The previous state of the window
    Qt::WindowStates m_window_prev_state;

    /// Flag to decide: close application or hide it to tray
    bool m_is_quit;

    QAction *m_toggle_task_shell_action;

    QAction *m_toggle_task_filter_action;

    QWidget *m_window;
    QGridLayout *m_layout;
    SystemTrayIcon *m_tray_icon;
    TasksView *m_tasks_view;
    QToolBar *m_task_toolbar;
    QLineEdit *m_task_shell;
    TagsEdit *m_task_filter;

    // Toolbar actions
    QAction *m_add_action;
    QAction *m_undo_action;
    QAction *m_update_action;
    QAction *m_done_action;
    QAction *m_edit_action;
    QAction *m_wait_action;
    QAction *m_delete_action;
    QAction *m_start_action;
    QAction *m_stop_action;

    std::unique_ptr<Taskwarrior> m_task_provider;

    TaskWatcher *m_task_watcher;
};

} // namespace ui

#endif // MAINWINDOW_HPP
