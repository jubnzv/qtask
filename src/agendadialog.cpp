#include "agendadialog.hpp"

#include <QApplication>
#include <QCalendarWidget>
#include <QColor>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDialog>
#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPalette>
#include <QTextCharFormat>
#include <QVBoxLayout>
#include <QWidget>

#include <utility>

#include "task.hpp"

AgendaDialog::AgendaDialog(QList<Task> tasks, QWidget *parent)
    : QDialog(parent)
    , m_calendar(new QCalendarWidget(this))
    , m_sched_tasks_list(new QListWidget(this))
    , m_due_tasks_list(new QListWidget(this))
    , m_tasks(std::move(tasks))
{
    setSizeGripEnabled(false);
    setWindowTitle(tr("Agenda"));
    initUI();
    resize(750, 480);
}

AgendaDialog::~AgendaDialog() = default;

void AgendaDialog::initUI()
{
    auto *main_layout = new QGridLayout(this);
    setLayout(main_layout);

    m_calendar->setSelectedDate(QDate::currentDate());
    setCalendarHighlight();
    main_layout->addWidget(m_calendar, 0, 0, 2, 1);

    m_sched_tasks_list->viewport()->setAutoFillBackground(false);
    main_layout->addLayout(
        CreateLabeledVerticalLayout(tr("Scheduled tasks:"), m_sched_tasks_list),
        0, 1);

    m_due_tasks_list->viewport()->setAutoFillBackground(false);
    main_layout->addLayout(
        CreateLabeledVerticalLayout(tr("Due tasks:"), m_due_tasks_list), 1, 1);

    connect(m_calendar, &QCalendarWidget::selectionChanged, this,
            &AgendaDialog::onUpdateTasks);
    onUpdateTasks();
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
    for (const auto &t : std::as_const(m_tasks)) {
        if (t.due.has_value()) {
            hightlight.setBackground(due_color);
            m_calendar->setDateTextFormat(t.due->date(), hightlight);
        } else if (t.sched.has_value()) {
            hightlight.setBackground(sched_color);
            m_calendar->setDateTextFormat(t.sched->date(), hightlight);
        }
    }
}

void AgendaDialog::onUpdateTasks()
{
    const auto date = m_calendar->selectedDate();
    m_sched_tasks_list->clear();
    m_due_tasks_list->clear();
    for (const auto &t : std::as_const(m_tasks)) {
        // Scheduled tasks
        if (t.sched.has_value() && t.sched->date() == date) {
            auto *task_item = new QListWidgetItem(m_sched_tasks_list);
            const auto text = QString{ "%1: %2" }.arg(t.uuid, t.description);
            task_item->setText(text);
            m_sched_tasks_list->addItem(task_item);
        }
        // Due tasks
        if (t.due.has_value() && t.due->date() == date) {
            auto *task_item = new QListWidgetItem(m_due_tasks_list);
            const auto text = QString{ "%1: %2" }.arg(t.uuid, t.description);
            task_item->setText(text);
            m_due_tasks_list->addItem(task_item);
        }
    }
}

QVBoxLayout *
AgendaDialog::CreateLabeledVerticalLayout(const QString &label_text,
                                          QWidget *widget)
{
    auto *layout = new QVBoxLayout(this);
    auto *label = new QLabel(label_text, this);
    layout->addWidget(label);
    layout->addWidget(widget);
    return layout;
}
