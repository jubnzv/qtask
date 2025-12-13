#pragma once

#include "taskproperty.hpp"

#include <QRect>
#include <QRectF>
#include <QString>

/// @brief Contains tag's text & tag's rectangle. Changes of the text are
/// tracked.
/// @note TagsRectsCalculator is used to populate rectangle stored, changes in
/// rectangle are not tracked.
class VisualTag {
  public:
    VisualTag(const QString &text, const QRect &rect)
        : m_text(text)
        , m_rect(rect)
    {
    }

    VisualTag() = default;

    [[nodiscard]]
    const QString &text() const
    {
        return m_text.get();
    }

    void setText(const QString &text)
    {
        m_text.modify([&text](auto &prop_value) { prop_value = text; });
    }

    [[nodiscard]]
    const QRect &rect() const
    {
        return m_rect;
    }

    void setRect(QRect rect) { m_rect = rect; }

    [[nodiscard]]
    bool isModified() const
    {
        return m_text.isModified();
    }

    [[nodiscard]]
    bool isEmpty() const
    {
        return m_text.get().isEmpty();
    }

    void setNotModified() { m_text.setNotModified(); }

  public:
    // FIXME: those are here to simulate QString and compile old code.
    void remove(qsizetype i, qsizetype len)
    {
        QString tmp = text();
        tmp.remove(i, len);
        setText(tmp);
    }

    void insert(qsizetype i, const QString &s)
    {
        QString tmp = text();
        tmp.insert(i, s);
        setText(tmp);
    }

    [[nodiscard]]
    qsizetype size() const
    {
        return text().size();
    }

  private:
    ModTrackingProperty<QString> m_text;
    QRect m_rect;
};
