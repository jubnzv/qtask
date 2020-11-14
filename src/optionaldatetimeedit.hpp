#ifndef OPTIONALDATETIMEEDIT_HPP
#define OPTIONALDATETIMEEDIT_HPP

#include <QCheckBox>
#include <QDateTimeEdit>
#include <QVariant>
#include <QWidget>

class OptionalDateTimeEdit : public QWidget {
    Q_OBJECT

  public:
    OptionalDateTimeEdit(const QString &label, const QDateTime &date,
                         QWidget *parent = nullptr);
    OptionalDateTimeEdit(const QString &label, QWidget *parent = nullptr);
    ~OptionalDateTimeEdit();

    QVariant getDateTime() const;
    void setDateTime(const QDateTime &dt);
    void setDateTime(const QVariant &);
    void setChecked(bool);

    void setMinimumDateTime(const QDateTime &);
    void setMaximumDateTime(const QDateTime &);

  private:
    void initUI(const QString &label);

  private:
    QDateTimeEdit *m_datetime_edit;
    QCheckBox *m_enabled;
};

#endif // OPTIONALDATETIMEEDIT_HPP
