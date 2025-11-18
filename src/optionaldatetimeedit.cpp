#include "optionaldatetimeedit.hpp"
#include "qtutil.hpp"

#include <QCheckBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <QString>
#include <QWidget>

#include <optional>

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

std::optional<QDateTime> OptionalDateTimeEdit::getDateTime() const
{
    return (m_enabled->isChecked())
               ? std::make_optional(m_datetime_edit->dateTime())
               : std::nullopt;
}

void OptionalDateTimeEdit::setDateTime(const QDateTime &dt)
{
    m_datetime_edit->setDateTime(dt);
}

void OptionalDateTimeEdit::setDateTime(const std::optional<QDateTime> &dt_opt)
{
    setChecked(dt_opt.has_value());
    if (dt_opt) {
        m_datetime_edit->setDateTime(*dt_opt);
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
