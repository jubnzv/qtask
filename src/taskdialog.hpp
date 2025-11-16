#ifndef TASKDIALOG_HPP
#define TASKDIALOG_HPP

#include <QBoxLayout>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QKeyEvent>
#include <QObject>
#include <QPushButton>
#include <QString>
#include <QTextEdit>
#include <QWidget>
#include <qtmetamacros.h>

#include "optionaldatetimeedit.hpp"
#include "tagsedit.hpp"
#include "task.hpp"

class TaskDialogBase : public QDialog {
    Q_OBJECT
  public:
    explicit TaskDialogBase(QWidget *parent = nullptr);
    ~TaskDialogBase() override = default;
    TaskDialogBase(const TaskDialogBase &) = delete;
    TaskDialogBase &operator=(const TaskDialogBase &) = delete;
    TaskDialogBase(TaskDialogBase &&) = delete;
    TaskDialogBase &operator=(TaskDialogBase &&) = delete;

    Task getTask();

  protected slots:
    virtual void onDescriptionChanged() = 0;

  protected:
    // Create horizontal layout of 3 buttons. Positive and negative buttons are
    // aligned according to the system (desktop) settings.
    QHBoxLayout *Create3ButtonsLayout(QPushButton *positive_button,
                                      QPushButton *negative_button,
                                      QPushButton *mid_button);

    void keyPressEvent(QKeyEvent *) override;
    void setTask(const Task &task);

    QVBoxLayout *const m_main_layout;
    QTextEdit *const m_task_description;
    QComboBox *const m_task_priority;
    QLineEdit *const m_task_project;
    TagsEdit *const m_task_tags;
    OptionalDateTimeEdit *const m_task_sched;
    OptionalDateTimeEdit *const m_task_due;
    OptionalDateTimeEdit *const m_task_wait;
    QString m_task_uuid;

  private:
    void constructUi();
};

class AddTaskDialog final : public TaskDialogBase {
    Q_OBJECT

  public:
    explicit AddTaskDialog(const QVariant &default_project = {},
                           QWidget *parent = nullptr);
    ~AddTaskDialog() final = default;

  public slots:
    void acceptContinue();

  signals:
    void createTaskAndContinue();

  protected slots:
    void onDescriptionChanged() final;

  private:
    void constructUi();

    QPushButton *const m_ok_btn;
    QPushButton *const m_continue_btn;
};

class EditTaskDialog final : public TaskDialogBase {
    Q_OBJECT
  public:
    explicit EditTaskDialog(const Task &, QWidget *parent = nullptr);
    ~EditTaskDialog() final = default;

  signals:
    void deleteTask(const QString &uuid);

  protected slots:
    void onDescriptionChanged() final;

  private:
    void constructUi();
    void requestDeleteTask();

    QPushButton *const m_ok_btn;
    QPushButton *const m_delete_btn;
};

#endif // TASKDIALOG_HPP
