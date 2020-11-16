#include "taskdialog.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDialog>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include "optionaldatetimeedit.hpp"
#include "task.hpp"
#include "tasksmodel.hpp"

TaskDialog::TaskDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QCoreApplication::applicationName() + " - Add task");
    initUI();
}

TaskDialog::TaskDialog(const Task &task, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QCoreApplication::applicationName() + " - Edit task");
    initUI();
    setTask(task);
}

TaskDialog::~TaskDialog() {}

Task TaskDialog::getTask()
{
    Task task;

    task.description = m_task_description->toPlainText();
    task.priority = Task::priorityFromString(m_task_priority->currentText());
    task.project = m_task_project->text();
    task.tags = m_task_tags->getTags();

    task.sched = m_task_sched->getDateTime();
    task.due = m_task_due->getDateTime();
    task.wait = m_task_wait->getDateTime();

    return task;
}

void TaskDialog::initUI()
{
    setWindowIcon(QIcon(":/icons/jtask.svg"));

    QLabel *description_label = new QLabel("Description:");
    m_task_description = new QTextEdit();
    m_task_description->setTabChangesFocus(true);

    QLabel *priority_label = new QLabel("Priority:");
    m_task_priority = new QComboBox();
    m_task_priority->addItem("");
    m_task_priority->addItem("L");
    m_task_priority->addItem("M");
    m_task_priority->addItem("H");

    QLabel *project_label = new QLabel("Project:");
    m_task_project = new QLineEdit();

    QLabel *tags_label = new QLabel("Tags:");
    m_task_tags = new TagsEdit();

    m_task_sched = new OptionalDateTimeEdit("Schedule:");
    m_task_sched->setChecked(false);
    // Taskwarrior's implementation feature
    m_task_sched->setMinimumDateTime(QDate(1980, 1, 2).startOfDay());
    m_task_sched->setMaximumDateTime(QDate(2038, 1, 1).startOfDay());
    m_task_sched->setDateTime(QDate::currentDate().startOfDay().addDays(1));

    m_task_due = new OptionalDateTimeEdit("Due:");
    m_task_due->setChecked(false);
    m_task_due->setMinimumDateTime(QDate(1980, 1, 2).startOfDay());
    m_task_due->setMaximumDateTime(QDate(2038, 1, 1).startOfDay());
    m_task_due->setDateTime(QDate::currentDate().startOfDay().addDays(5));

    m_task_wait = new OptionalDateTimeEdit("Wait:");
    m_task_wait->setChecked(false);
    m_task_wait->setMinimumDateTime(QDate(1980, 1, 2).startOfDay());
    m_task_wait->setMaximumDateTime(QDate(2038, 1, 1).startOfDay());
    m_task_wait->setDateTime(QDate::currentDate().startOfDay().addDays(5));

    m_btn_box =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_btn_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_btn_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QGridLayout *grid_layout = new QGridLayout();
    grid_layout->addWidget(priority_label, 0, 0);
    grid_layout->addWidget(m_task_priority, 0, 1);
    grid_layout->addWidget(project_label, 1, 0);
    grid_layout->addWidget(m_task_project, 1, 1);
    grid_layout->addWidget(tags_label, 2, 0);
    grid_layout->addWidget(m_task_tags, 2, 1);
    grid_layout->addWidget(m_task_sched, 3, 0, 1, 2);
    grid_layout->addWidget(m_task_due, 4, 0, 1, 2);
    grid_layout->addWidget(m_task_wait, 5, 0, 1, 2);

    QVBoxLayout *main_layout = new QVBoxLayout();
    main_layout->addWidget(description_label);
    main_layout->addWidget(m_task_description);
    main_layout->addLayout(grid_layout);
    main_layout->addWidget(m_btn_box);
    main_layout->setContentsMargins(5, 5, 5, 5);

    setLayout(main_layout);
}

void TaskDialog::setTask(const Task &task)
{
    m_task_description->setText(task.description);
    m_task_project->setText(task.project);
    m_task_tags->setTags(task.tags);

    m_task_sched->setDateTime(task.sched);
    m_task_due->setDateTime(task.due);
    m_task_wait->setDateTime(task.wait);

    switch (task.priority) {
    case Task::Priority::Unset:
        m_task_priority->setCurrentIndex(0);
        break;
    case Task::Priority::L:
        m_task_priority->setCurrentIndex(1);
        break;
    case Task::Priority::M:
        m_task_priority->setCurrentIndex(2);
        break;
    case Task::Priority::H:
        m_task_priority->setCurrentIndex(3);
        break;
    }
}
