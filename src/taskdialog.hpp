#ifndef TASKDIALOG_HPP
#define TASKDIALOG_HPP

#include <memory>

#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTextEdit>
#include <QWidget>

#include "filterlineedit.hpp"
#include "optionaldatetimeedit.hpp"
#include "task.hpp"
#include "taskwarrior.hpp"

class TaskDialog : public QDialog {
    Q_OBJECT

  public:
    TaskDialog(QWidget *parent = nullptr);
    TaskDialog(const Task &, QWidget *parent = nullptr);
    ~TaskDialog();

    Task getTask();

  private:
    void initUI();
    void setTask(const Task &task);

  private:
    QTextEdit *m_task_description;
    QComboBox *m_task_priority;
    QLineEdit *m_task_project;
    FilterLineEdit *m_task_tags;
    OptionalDateTimeEdit *m_task_sched;
    OptionalDateTimeEdit *m_task_due;
    OptionalDateTimeEdit *m_task_wait;
    QDialogButtonBox *m_btn_box;
};

#endif // TASKDIALOG_HPP
