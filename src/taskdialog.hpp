#ifndef TASKDIALOG_HPP
#define TASKDIALOG_HPP

#include <QBoxLayout>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QKeyEvent>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

#include "optionaldatetimeedit.hpp"
#include "tagsedit.hpp"
#include "task.hpp"
#include "taskwarrior.hpp"

class TaskDialog : public QDialog {
    Q_OBJECT

  public:
    explicit TaskDialog(QWidget *parent = nullptr);
    virtual ~TaskDialog() = default;

    Task getTask();

  protected:
    void keyPressEvent(QKeyEvent *) override;

    void initUI();
    void setTask(const Task &task);

  protected slots:
    virtual void onDescriptionChanged() = 0;

  protected:
    QVBoxLayout *m_main_layout;
    QTextEdit *m_task_description;
    QComboBox *m_task_priority;
    QLineEdit *m_task_project;
    TagsEdit *m_task_tags;
    OptionalDateTimeEdit *m_task_sched;
    OptionalDateTimeEdit *m_task_due;
    OptionalDateTimeEdit *m_task_wait;

    QString m_task_uuid = "";
};

class AddTaskDialog final : public TaskDialog {
    Q_OBJECT

  public:
    explicit AddTaskDialog(const QVariant &default_project = {}, QWidget *parent = nullptr);
    ~AddTaskDialog() = default;

  protected slots:
    void onDescriptionChanged() override;

  private:
    void initUI();

  public slots:
    void acceptContinue();

  signals:
    void createTaskAndContinue();

  private:
    QPushButton *m_ok_btn;
    QPushButton *m_continue_btn;
};

class EditTaskDialog final : public TaskDialog {
    Q_OBJECT

  public:
    explicit EditTaskDialog(const Task &, QWidget *parent = nullptr);
    ~EditTaskDialog() = default;

  protected slots:
    void onDescriptionChanged() override;

  private:
    void initUI();
    void requestDeleteTask();

  signals:
    void deleteTask(const QString &uuid);

  private:
    QPushButton *m_ok_btn;
    QPushButton *m_delete_btn;
};

#endif // TASKDIALOG_HPP
