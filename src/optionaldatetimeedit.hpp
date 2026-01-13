#ifndef OPTIONALDATETIMEEDIT_HPP
#define OPTIONALDATETIMEEDIT_HPP

#include <QCheckBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QObject>
#include <QString>
#include <QWidget>

#include "qtutil.hpp"
#include "task_date_time.hpp"

class OptionalDateTimeEdit : public QWidget {
    Q_OBJECT

  public:
    OptionalDateTimeEdit(const QString &label, const QDateTime &date,
                         QWidget *parent = nullptr);
    explicit OptionalDateTimeEdit(const QString &label,
                                  QWidget *parent = nullptr);

    template <ETaskDateTimeRole taRole>
    TaskDateTime<taRole> getDateTime() const
    {
        if (!m_enabled->isChecked()) {
            return TaskDateTime<taRole>();
        }
        QDateTime dt = m_datetime_edit->dateTime();
        dt.setTime(QTime(dt.time().hour(), dt.time().minute(), 0, 0));
        return TaskDateTime<taRole>(dt);
    }
    void setDateTime(const QDateTime &dt);

    template <ETaskDateTimeRole taRole>
    void setDateTime(const TaskDateTime<taRole> &dt_opt)
    {
        setChecked(dt_opt.has_value());
        if (dt_opt.has_value()) {
            m_datetime_edit->setDateTime(*dt_opt);
        }
    }
    void setChecked(bool);

    void setMinimumDateTime(const QDateTime &);
    void setMaximumDateTime(const QDateTime &);

    template <ETaskDateTimeRole taRole>
    void setupDateTimeRole()
    {
        setChecked(false);
        setMinimumDateTime(startOfDay(QDate(1980, 1, 2)));
        setMaximumDateTime(startOfDay(QDate(2038, 1, 1)));
        setDateTime(TaskDateTime<taRole>::suggest_default_date_time());
    }

  private:
    QDateTimeEdit *const m_datetime_edit;
    QCheckBox *const m_enabled;
};

#endif // OPTIONALDATETIMEEDIT_HPP
