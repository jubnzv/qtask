#ifndef TASKDIALOG_HPP
#define TASKDIALOG_HPP

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
    TaskDialog(QWidget *parent = nullptr);
    TaskDialog(const Task &, QWidget *parent = nullptr);
    ~TaskDialog();

    Task getTask();

  protected:
    void keyPressEvent(QKeyEvent *) override;

  private:
    void initUI();
    void setTask(const Task &task);

  public slots:
    void acceptContinue();

  private slots:
    void onDescriptionChanged();

  signals:
    void createTaskAndContinue();

  private:
    QTextEdit *m_task_description;
    QComboBox *m_task_priority;
    QLineEdit *m_task_project;
    TagsEdit *m_task_tags;
    OptionalDateTimeEdit *m_task_sched;
    OptionalDateTimeEdit *m_task_due;
    OptionalDateTimeEdit *m_task_wait;
    QPushButton *m_ok_btn;
    QPushButton *m_continue_btn;
};

#endif // TASKDIALOG_HPP
