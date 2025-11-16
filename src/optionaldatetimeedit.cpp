#include "optionaldatetimeedit.hpp"
#include "qtutil.hpp"

#include <QCheckBox>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <QString>
#include <QVariant>
#include <QWidget>

OptionalDateTimeEdit::OptionalDateTimeEdit(const QString &label,
                                           const QDateTime &dt, QWidget *parent)
    : QWidget(parent)
    , m_datetime_edit(new QDateTimeEdit())
    , m_enabled(new QCheckBox(label, this))
{
    {
        auto layout = new QHBoxLayout(this);
        connect(m_enabled, &QCheckBox::stateChanged, this,
                [&]() { m_datetime_edit->setEnabled(m_enabled->isChecked()); });
        layout->addWidget(m_enabled);

        m_datetime_edit->setCalendarPopup(true);
        layout->addWidget(m_datetime_edit);
        setLayout(layout);
    }
    setDateTime(dt);
    setChecked(true);
}

OptionalDateTimeEdit::OptionalDateTimeEdit(const QString &label,
                                           QWidget *parent)
    : OptionalDateTimeEdit(label, startOfDay(QDate{ 1970, 1, 1 }), parent)
{
}

OptionalDateTimeEdit::~OptionalDateTimeEdit() = default;

QVariant OptionalDateTimeEdit::getDateTime() const
{
    return (m_enabled->isChecked()) ? QVariant{ m_datetime_edit->dateTime() }
                                    : QVariant{};
}

void OptionalDateTimeEdit::setDateTime(const QDateTime &dt)
{
    m_datetime_edit->setDateTime(dt);
}

void OptionalDateTimeEdit::setDateTime(const QVariant &dt_opt)
{
    if (dt_opt.isNull()) {
        setChecked(false);
    } else {
        m_datetime_edit->setDateTime(dt_opt.toDateTime());
        setChecked(true);
    }
}

void OptionalDateTimeEdit::setChecked(bool state)
{
    m_enabled->setChecked(state);
}

void OptionalDateTimeEdit::setMinimumDateTime(const QDateTime &dt)
{
    m_datetime_edit->setMinimumDateTime(dt);
}

void OptionalDateTimeEdit::setMaximumDateTime(const QDateTime &dt)
{
    m_datetime_edit->setMaximumDateTime(dt);
}
