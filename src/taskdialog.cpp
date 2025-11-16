#include "taskdialog.hpp"

#include <QApplication>
#include <QBoxLayout>
#include <QChar>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDialog>
#include <QGridLayout>
#include <QImage>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QShortcut>
#include <QStringList>
#include <QStyle>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <Qt>
#include <qtmetamacros.h>
#include <qtversionchecks.h>

#include "optionaldatetimeedit.hpp"
#include "qtutil.hpp"
#include "tagsedit.hpp"
#include "task.hpp"

namespace
{
/**
 * * @returns true if Ok-Cancel order of the buttons should be used (Ok/Positive
 * is 1st).
 */
bool IsOkCancelOrder()
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const int layout =
        QApplication::style()->styleHint(QStyle::SH_DialogButtonLayout);
    if (layout == static_cast<int>(Qt::LeftToRight)) {
        return true;
    }
    return false;

#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    const QStyle::LayoutDirection layout =
        QApplication::style()->standardButtonLayout();

    if (layout == QStyle::LeftToRight || layout == QStyle::WindowsLayout) {
        return true;
    }
    return false;

#else
    // Fallback to "Cancel" is 1st
    return false;
#endif
}
} // namespace

TaskDialogBase::TaskDialogBase(QWidget *parent)
    : QDialog(parent)
    , m_main_layout(new QVBoxLayout(this))
    , m_task_description(new QTextEdit(this))
    , m_task_priority(new QComboBox(this))
    , m_task_project(new QLineEdit(this))
    , m_task_tags(new TagsEdit(this))
    , m_task_sched(new OptionalDateTimeEdit(tr("Schedule:"), this))
    , m_task_due(new OptionalDateTimeEdit(tr("Due:"), this))
    , m_task_wait(new OptionalDateTimeEdit(tr("Wait:"), this))
    , m_task_uuid("")
{
    setWindowIcon(QIcon(":/icons/qtask.svg"));
    constructUi();
}

void TaskDialogBase::constructUi()
{
    auto *description_label = new QLabel(tr("Description:"), this);
    m_task_description->setTabChangesFocus(true);
    QObject::connect(m_task_description, &QTextEdit::textChanged, this,
                     &TaskDialogBase::onDescriptionChanged);

    auto *priority_label = new QLabel(tr("Priority:"), this);
    m_task_priority->addItem("");
    m_task_priority->addItem("L");
    m_task_priority->addItem("M");
    m_task_priority->addItem("H");

    auto *project_label = new QLabel(tr("Project:"), this);

    QObject::connect(m_task_project, &QLineEdit::editingFinished, this,
                     [&]() { focusNextChild(); });

    auto *tags_label = new QLabel(tr("Tags:"), this);

    m_task_sched->setChecked(false);
    // Taskwarrior's implementation feature
    m_task_sched->setMinimumDateTime(startOfDay(QDate(1980, 1, 2)));
    m_task_sched->setMaximumDateTime(startOfDay(QDate(2038, 1, 1)));
    m_task_sched->setDateTime(startOfDay(QDate::currentDate()).addDays(1));

    m_task_due->setChecked(false);
    m_task_due->setMinimumDateTime(startOfDay(QDate(1980, 1, 2)));
    m_task_due->setMaximumDateTime(startOfDay(QDate(2038, 1, 1)));
    m_task_due->setDateTime(startOfDay(QDate::currentDate()).addDays(5));

    m_task_wait->setChecked(false);
    m_task_wait->setMinimumDateTime(startOfDay(QDate(1980, 1, 2)));
    m_task_wait->setMaximumDateTime(startOfDay(QDate(2038, 1, 1)));
    m_task_wait->setDateTime(startOfDay(QDate::currentDate()).addDays(5));

    auto *grid_layout = new QGridLayout(this);
    grid_layout->addWidget(priority_label, 0, 0);
    grid_layout->addWidget(m_task_priority, 0, 1);
    grid_layout->addWidget(project_label, 1, 0);
    grid_layout->addWidget(m_task_project, 1, 1);
    grid_layout->addWidget(tags_label, 2, 0);
    grid_layout->addWidget(m_task_tags, 2, 1);
    grid_layout->addWidget(m_task_sched, 3, 0, 1, 2);
    grid_layout->addWidget(m_task_due, 4, 0, 1, 2);
    grid_layout->addWidget(m_task_wait, 5, 0, 1, 2);

    m_main_layout->addWidget(description_label);
    m_main_layout->addWidget(m_task_description);
    m_main_layout->addLayout(grid_layout);
    m_main_layout->setContentsMargins(5, 5, 5, 5);

    setLayout(m_main_layout);
}

Task TaskDialogBase::getTask()
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
        if (!t.isEmpty()) {
            task.tags.push_back(t);
        }
    }

    task.sched = m_task_sched->getDateTime();
    task.due = m_task_due->getDateTime();
    task.wait = m_task_wait->getDateTime();

    return task;
}

QHBoxLayout *TaskDialogBase::Create3ButtonsLayout(QPushButton *positive_button,
                                                  QPushButton *negative_button,
                                                  QPushButton *mid_button)
{
    auto *button_layout = new QHBoxLayout(this);
    if (IsOkCancelOrder()) {
        button_layout->addWidget(positive_button);
        button_layout->addWidget(mid_button);
        button_layout->addWidget(negative_button);

    } else {
        button_layout->addWidget(negative_button);
        button_layout->addWidget(mid_button);
        button_layout->addWidget(positive_button);
    }
    return button_layout;
}

void TaskDialogBase::keyPressEvent(QKeyEvent *event)
{
    if (event == QKeySequence::Close) {
        reject();
    } else {
        event->ignore();
    }
}

void TaskDialogBase::setTask(const Task &task)
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

    m_task_uuid = task.uuid;
}

AddTaskDialog::AddTaskDialog(const QVariant &default_project, QWidget *parent)
    : TaskDialogBase(parent)
    , m_ok_btn(new QPushButton(
          QApplication::style()->standardIcon(QStyle::SP_DialogOkButton),
          tr("Ok"), this))
    , m_continue_btn(new QPushButton(tr("Continue"), this))
{
    setWindowTitle(tr("Add task"));
    constructUi();
    if (!default_project.isNull()) {
        m_task_project->setText(default_project.toString());
    }
    adjustSize();
}

void AddTaskDialog::constructUi()
{
    m_ok_btn->setEnabled(false);
    QObject::connect(new QShortcut(QKeySequence("Ctrl+Return"), this),
                     &QShortcut::activated, this, &QDialog::accept);
    m_ok_btn->setToolTip(tr("Create task"));

    m_continue_btn->setEnabled(false);
    QObject::connect(new QShortcut(QKeySequence("Alt+Return"), this),
                     &QShortcut::activated, this,
                     &AddTaskDialog::createTaskAndContinue);
    m_continue_btn->setToolTip(tr("Create task and continue"));

    auto *cancel_btn = new QPushButton(
        QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton),
        tr("Cancel"), this);

    QObject::connect(new QShortcut(QKeySequence::Cancel, this),
                     &QShortcut::activated, this, &QDialog::reject);
    cancel_btn->setToolTip(tr("Cancel and close this window"));

    connect(m_ok_btn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_continue_btn, &QPushButton::clicked, this,
            &AddTaskDialog::createTaskAndContinue);
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);

    m_main_layout->addLayout(
        Create3ButtonsLayout(m_ok_btn, cancel_btn, m_continue_btn));
}

void AddTaskDialog::acceptContinue()
{
    m_task_description->setText("");
    m_task_description->update();
    m_task_description->setFocus();
}

void AddTaskDialog::onDescriptionChanged()
{
    if (m_task_description->toPlainText().isEmpty()) {
        m_ok_btn->setEnabled(false);
        m_continue_btn->setEnabled(false);
    } else {
        m_ok_btn->setEnabled(true);
        m_continue_btn->setEnabled(true);
    }
}

EditTaskDialog::EditTaskDialog(const Task &task, QWidget *parent)
    : TaskDialogBase(parent)
    , m_ok_btn(new QPushButton(
          QApplication::style()->standardIcon(QStyle::SP_DialogOkButton),
          tr("Ok"), this))
    , m_delete_btn(new QPushButton(tr("Delete"), this))
{
    setWindowTitle(tr("Edit task"));
    constructUi();
    setTask(task);
}

void EditTaskDialog::constructUi()
{
    QObject::connect(new QShortcut(QKeySequence("Ctrl+Return"), this),
                     &QShortcut::activated, this, &QDialog::accept);
    m_ok_btn->setToolTip(tr("Create task"));

    QObject::connect(new QShortcut(QKeySequence("Ctrl+Delete"), this),
                     &QShortcut::activated, this,
                     &EditTaskDialog::requestDeleteTask);
    m_delete_btn->setToolTip(tr("Delete this task"));

    auto *cancel_btn = new QPushButton(
        QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton),
        tr("Cancel"), this);
    QObject::connect(new QShortcut(QKeySequence::Cancel, this),
                     &QShortcut::activated, this, &QDialog::reject);
    cancel_btn->setToolTip(tr("Cancel and close this window"));

    connect(m_ok_btn, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_delete_btn, &QPushButton::clicked, this,
            &EditTaskDialog::requestDeleteTask);
    connect(cancel_btn, &QPushButton::clicked, this, &QDialog::reject);

    m_main_layout->addLayout(
        Create3ButtonsLayout(m_ok_btn, cancel_btn, m_delete_btn));
}

void EditTaskDialog::requestDeleteTask()
{
    auto reply = QMessageBox::question(this, tr("Conifrm action"),
                                       tr("Delete task #%1?").arg(m_task_uuid),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        emit deleteTask(m_task_uuid);
        reject();
    }
}

void EditTaskDialog::onDescriptionChanged()
{
    m_ok_btn->setEnabled(!m_task_description->toPlainText().isEmpty());
}
