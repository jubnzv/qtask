#include "taskdialog.hpp"

#include <QApplication>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDialog>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
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

    auto project = m_task_project->text();
    project.replace("pro:", "");
    project.replace("project:", "");
    task.project = project;

    for (const auto &tag : m_task_tags->getTags()) {
        QString t(tag);
        t.remove(QChar('+'));
        if (!t.isEmpty())
            task.tags.push_back(t);
    }

    task.sched = m_task_sched->getDateTime();
    task.due = m_task_due->getDateTime();
    task.wait = m_task_wait->getDateTime();

    return task;
}

void TaskDialog::keyPressEvent(QKeyEvent *event)
{
    if (event == QKeySequence::Close) {
        reject();
    } else {
        event->ignore();
    }
}

void TaskDialog::initUI()
{
    setWindowIcon(QIcon(":/icons/jtask.svg"));

    QLabel *description_label = new QLabel("Description:");
    m_task_description = new QTextEdit();
    m_task_description->setTabChangesFocus(true);
    QObject::connect(m_task_description, &QTextEdit::textChanged, this,
                     &TaskDialog::onDescriptionChanged);

    QLabel *priority_label = new QLabel("Priority:");
    m_task_priority = new QComboBox();
    m_task_priority->addItem("");
    m_task_priority->addItem("L");
    m_task_priority->addItem("M");
    m_task_priority->addItem("H");

    QLabel *project_label = new QLabel("Project:");
    m_task_project = new QLineEdit();
    QObject::connect(m_task_project, &QLineEdit::editingFinished, this,
                     [&]() { focusNextChild(); });

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

    m_ok_btn = new QPushButton(
        QApplication::style()->standardIcon(QStyle::SP_DialogOkButton),
        tr("Ok"), this);
    m_ok_btn->setEnabled(false);
    auto *create_shortcut = new QShortcut(QKeySequence("Ctrl+Return"), this);
    QObject::connect(create_shortcut, &QShortcut::activated, this,
                     &QDialog::accept);
    m_ok_btn->setToolTip(tr("Create task"));

    m_continue_btn = new QPushButton(tr("Continue"), this);
    m_continue_btn->setEnabled(false);
    auto *continue_shortcut = new QShortcut(QKeySequence("Alt+Return"), this);
    QObject::connect(continue_shortcut, &QShortcut::activated, this,
                     &TaskDialog::createTaskAndContinue);
    m_continue_btn->setToolTip(tr("Create task and continue"));

    auto *cancel_btn = new QPushButton(
        QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton),
        tr("Cancel"), this);
    cancel_btn->setToolTip(tr("Cancel and close window"));

    QBoxLayout *button_layout = new QHBoxLayout();
    button_layout->addWidget(cancel_btn);
    button_layout->addWidget(m_continue_btn);
    button_layout->addWidget(m_ok_btn);

    connect(m_ok_btn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_continue_btn, &QPushButton::clicked, this,
            &TaskDialog::createTaskAndContinue);
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);

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
    main_layout->addLayout(button_layout);
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

void TaskDialog::acceptContinue()
{
    m_task_description->setText("");
    m_task_description->update();
    m_task_description->setFocus();
}

void TaskDialog::onDescriptionChanged()
{
    if (m_task_description->toPlainText().isEmpty()) {
        m_ok_btn->setEnabled(false);
        m_continue_btn->setEnabled(false);
    } else {
        m_ok_btn->setEnabled(true);
        m_continue_btn->setEnabled(true);
    }
}
