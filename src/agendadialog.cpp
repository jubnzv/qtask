#include "agendadialog.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPalette>
#include <QTextCharFormat>
#include <QVBoxLayout>

#include "task.hpp"

AgendaDialog::AgendaDialog(const QList<Task> &tasks, QWidget *parent)
    : QDialog(parent)
    , m_tasks(tasks)
{
    initUI();
}

AgendaDialog::~AgendaDialog() {}

void AgendaDialog::initUI()
{
    setSizeGripEnabled(false);
    resize(640, 480);

    setWindowTitle(QCoreApplication::applicationName() + " - Agenda");

    m_calendar = new QCalendarWidget(this);
    m_calendar->setSelectedDate(QDate::currentDate());
    setCalendarHighlight();

    auto *sched_layout = new QVBoxLayout;
    auto *sched_label = new QLabel(tr("Scheduled tasks:"));
    m_sched_tasks_list = new QListWidget(this);
    m_sched_tasks_list->viewport()->setAutoFillBackground(false);
    sched_layout->addWidget(sched_label);
    sched_layout->addWidget(m_sched_tasks_list);

    auto *due_layout = new QVBoxLayout;
    auto *due_label = new QLabel(tr("Due tasks:"));
    m_due_tasks_list = new QListWidget(this);
    m_due_tasks_list->viewport()->setAutoFillBackground(false);
    due_layout->addWidget(due_label);
    due_layout->addWidget(m_due_tasks_list);

    connect(m_calendar, &QCalendarWidget::selectionChanged, this,
            &AgendaDialog::onUpdateTasks);
    onUpdateTasks();

    auto *main_layout = new QGridLayout(this);
    main_layout->addWidget(m_calendar, 0, 0, 2, 1);
    main_layout->addLayout(sched_layout, 0, 1);
    main_layout->addLayout(due_layout, 1, 1);
}

void AgendaDialog::setCalendarHighlight()
{
    const auto group = QPalette::Active;
    const auto role = QPalette::Window;
    auto select_color = [](const int &c) { return (c < 255) ? c : c % 255; };
    auto palette = QApplication::palette();
    QColor sched_color = palette.color(group, role).name();
    sched_color.setRed(select_color(sched_color.red() + 150));
    sched_color.setGreen(select_color(sched_color.green() - 25));
    QColor due_color = palette.color(group, role).name();
    due_color.setRed(select_color(due_color.red() + 15));
    due_color.setGreen(select_color(due_color.green() - 25));
    QTextCharFormat hightlight;
    for (const auto &t : m_tasks) {
        if (!t.due.isNull()) {
            hightlight.setBackground(due_color);
            m_calendar->setDateTextFormat(t.due.toDate(), hightlight);
        } else if (!t.sched.isNull()) {
            hightlight.setBackground(sched_color);
            m_calendar->setDateTextFormat(t.sched.toDate(), hightlight);
        }
    }
}

void AgendaDialog::onUpdateTasks()
{
    const auto date = m_calendar->selectedDate();
    m_sched_tasks_list->clear();
    m_due_tasks_list->clear();
    for (const auto &t : m_tasks) {
        // Scheduled tasks
        if (!t.sched.isNull() && t.sched.toDate() == date) {
            QListWidgetItem *task_item = new QListWidgetItem;
            const auto text = QString{ "%1: %2" }.arg(t.uuid, t.description);
            task_item->setText(text);
            m_sched_tasks_list->addItem(task_item);
        }
        // Due tasks
        if (!t.due.isNull() && t.due.toDate() == date) {
            QListWidgetItem *task_item = new QListWidgetItem;
            const auto text = QString{ "%1: %2" }.arg(t.uuid, t.description);
            task_item->setText(text);
            m_due_tasks_list->addItem(task_item);
        }
    }
}
