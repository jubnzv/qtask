#ifndef OPTIONALDATETIMEEDIT_HPP
#define OPTIONALDATETIMEEDIT_HPP

#include <QCheckBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QObject>
#include <QString>
#include <QWidget>

#include <optional>

class OptionalDateTimeEdit : public QWidget {
    Q_OBJECT

  public:
    OptionalDateTimeEdit(const QString &label, const QDateTime &date,
                         QWidget *parent = nullptr);
    OptionalDateTimeEdit(const QString &label, QWidget *parent = nullptr);
    ~OptionalDateTimeEdit() override;

    std::optional<QDateTime> getDateTime() const;
    void setDateTime(const QDateTime &dt);
    void setDateTime(const std::optional<QDateTime> &);
    void setChecked(bool);

    void setMinimumDateTime(const QDateTime &);
    void setMaximumDateTime(const QDateTime &);

  private:
    QDateTimeEdit *const m_datetime_edit;
    QCheckBox *const m_enabled;
};

#endif // OPTIONALDATETIMEEDIT_HPP
