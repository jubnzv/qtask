#include "datetimedialog.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <qnamespace.h>
#include <utility>

DateTimeDialog::DateTimeDialog(QWidget *parent)
    : DateTimeDialog(QDateTime::currentDateTime(), parent)
{
}

DateTimeDialog::DateTimeDialog(QDateTime datetime, QWidget *parent)
    : QDialog(parent)
    , m_calendar(new QCalendarWidget(this))
    , m_datetime_edit(new QDateTimeEdit(this))
    , m_datetime(std::move(datetime))
{
    setSizeGripEnabled(false);
    resize(260, 230);
    setWindowTitle(tr("Select date"));

    auto *buttons = new QDialogButtonBox(this);
    {
        auto *vl = new QVBoxLayout(this);
        vl->setObjectName(QString::fromUtf8("verticalLayout"));
        vl->setContentsMargins(0, 0, 0, 0);
        vl->addWidget(m_calendar);
        vl->addWidget(m_datetime_edit);
        vl->addWidget(buttons);
    }

    m_calendar->setObjectName(QString::fromUtf8("m_calendar"));
    m_calendar->setSelectedDate(m_datetime.date());
    connect(m_calendar, &QCalendarWidget::selectionChanged, this, [&]() {
        m_datetime_edit->setDate(m_calendar->selectedDate());
        m_datetime = m_datetime_edit->dateTime();
    });

    m_datetime_edit->setObjectName(QString::fromUtf8("m_datetime_edit"));
    m_datetime_edit->setDateTime(m_datetime);

    connect(m_datetime_edit, &QDateTimeEdit::dateTimeChanged, this, [&]() {
        m_datetime = m_datetime_edit->dateTime();
        m_calendar->setSelectedDate(m_datetime.date());
    });

    buttons->setObjectName(QString::fromUtf8("buttons"));
    buttons->setOrientation(Qt::Horizontal);
    buttons->setStandardButtons(QDialogButtonBox::Cancel |
                                QDialogButtonBox::Ok);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

DateTimeDialog::~DateTimeDialog() = default;

const QDateTime &DateTimeDialog::getDateTime() const { return m_datetime; }
