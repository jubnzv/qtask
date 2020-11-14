#include "datetimedialog.hpp"

#include <QCoreApplication>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QVBoxLayout>

DateTimeDialog::DateTimeDialog(QWidget *parent)
    : QDialog(parent)
    , m_datetime(QDateTime::currentDateTime())
{
    initUI();
}

DateTimeDialog::DateTimeDialog(const QDateTime &datetime, QWidget *parent)
    : QDialog(parent)
    , m_datetime(datetime)
{
    initUI();
}

DateTimeDialog::~DateTimeDialog() {}

QDateTime DateTimeDialog::getDateTime() const { return m_datetime; }

void DateTimeDialog::initUI()
{
    setSizeGripEnabled(false);
    resize(260, 230);

    setWindowTitle(QCoreApplication::applicationName() + "Select date");

    auto *widget = new QWidget(this);
    widget->setObjectName(QString::fromUtf8("widget"));
    widget->setGeometry(QRect(0, 10, 258, 215));

    auto *vl = new QVBoxLayout(widget);
    vl->setObjectName(QString::fromUtf8("verticalLayout"));
    vl->setContentsMargins(0, 0, 0, 0);

    m_calendar = new QCalendarWidget(widget);
    m_calendar->setObjectName(QString::fromUtf8("m_calendar"));
    m_calendar->setSelectedDate(m_datetime.date());
    vl->addWidget(m_calendar);
    connect(m_calendar, &QCalendarWidget::selectionChanged, this, [&]() {
        m_datetime_edit->setDate(m_calendar->selectedDate());
        m_datetime = m_datetime_edit->dateTime();
    });

    m_datetime_edit = new QDateTimeEdit(widget);
    m_datetime_edit->setObjectName(QString::fromUtf8("m_datetime_edit"));
    m_datetime_edit->setDateTime(m_datetime);
    vl->addWidget(m_datetime_edit);
    connect(m_datetime_edit, &QDateTimeEdit::dateTimeChanged, this, [&]() {
        m_datetime = m_datetime_edit->dateTime();
        m_calendar->setSelectedDate(m_datetime.date());
    });

    auto *buttons = new QDialogButtonBox(widget);
    buttons->setObjectName(QString::fromUtf8("buttons"));
    buttons->setOrientation(Qt::Horizontal);
    buttons->setStandardButtons(QDialogButtonBox::Cancel |
                                QDialogButtonBox::Ok);
    vl->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}
