#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <memory>
#include <optional>

#include <QCloseEvent>
#include <QEvent>
#include <QGridLayout>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QTableView>
#include <QTimer>
#include <QVariant>

#include <qnamespace.h>
#include <qtmetamacros.h>
#include <qtypes.h>

#include "tagsedit.hpp"
#include "task.hpp"
#include "tasksmodel.hpp"
#include "tasksview.hpp"
#include "taskwarrior.hpp"
#include "trayicon.hpp"

namespace ui
{
class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    MainWindow();
    ~MainWindow() override;
    MainWindow(const MainWindow &) = delete;
    MainWindow &operator=(const MainWindow &) = delete;
    MainWindow(MainWindow &&) = delete;
    MainWindow &operator=(MainWindow &&) = delete;

  private:
    void initMainWindow();
    void initTasksTable();
    void initTrayIcon();
    void initFileMenu();
    void connectViewMenuActions();
    void initToolsMenu();
    void initHelpMenu();
    void initShortcuts();
    void connectTaskToolbarActions();
    void toggleMainWindow();
    void onOpenSettings(bool exitAppToo);
    void quitApp();

    bool eventFilter(QObject *watched, QEvent *event) override;
    void changeEvent(QEvent *) override;
    void closeEvent(QCloseEvent *event) override;

    [[nodiscard]] std::optional<QString> getSelectedTaskShortId() const;
    [[nodiscard]] QStringList getSelectedTaskShortIds() const;
    [[nodiscard]] QList<DetailedTaskInfo> getSelectedTaskInModel() const;
  public slots:
    /// Add entry to tags filter
    void pushFilterTag(const QString &);

    /// Show this window if minimized
    void showMainWindow();

    /// Receive a message from a secondary QTask instance
    void receiveNewInstanceMessage(quint32 instanceId,
                                   const QByteArray &message);

  private slots:
    void onToggleTaskShell(bool checked);
    void onSettingsMenu();
    void onAddTask();
    void onDeleteTasks();
    void onSetTasksDone();
    void onEnterTaskCommand();
    void onApplyFilter();
    void onEditTaskAction();
    void showEditTaskDialog(const QModelIndex &);

    void updateTaskToolbar();
  signals:
    void acceptContinueCreatingTasks();

  private:
    /// The previous state of the window
    Qt::WindowStates m_window_prev_state;

    /// Flag to decide: close application or hide it to tray
    bool m_is_quit;

    QWidget *const m_window;
    QGridLayout *const m_layout;
    SystemTrayIcon *const m_tray_icon;
    TasksView *const m_tasks_view;
    QToolBar *const m_task_toolbar;
    QLineEdit *const m_task_shell;
    TagsEdit *const m_task_filter;

    // Allocates and holds pointers to view menu actions.
    // Parent is set to this menu.
    struct TViewMenuActions {
        QPointer<QAction> m_toggle_task_shell_action;
        explicit TViewMenuActions(QMenu &parent);
    } const m_view_menu_actions;

    // Allocates and holds pointers for toolbar actions.
    // Parent is set to toolbar.
    // Note, any action can be addded to the toolbar later too.
    struct TToolbarActionsDefined {
        QPointer<QAction> m_add_action;
        QPointer<QAction> m_undo_action;
        QPointer<QAction> m_refresh_action;
        QPointer<QAction> m_done_action;
        QPointer<QAction> m_edit_action;
        QPointer<QAction> m_wait_action;
        QPointer<QAction> m_delete_action;
        QPointer<QAction> m_start_action;
        QPointer<QAction> m_stop_action;
        explicit TToolbarActionsDefined(QToolBar &parent);
    } const m_toolbar_actions;

    std::shared_ptr<Taskwarrior> m_task_provider;
    TasksModel *m_data_model;

    QTimer m_do_not_lock_ui;
};

} // namespace ui

#endif // MAINWINDOW_HPP
