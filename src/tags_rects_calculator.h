#pragma once

#include "predicate_skip_iterator.hpp"
#include "visual_tag.hpp"
#include <QFontMetrics>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QStyleOptionFrame>
#include <QTextLayout>
#include <QWidget>
#include <QtGlobal>
#include <QtVersionChecks>

#include <functional>
#include <iterator>
#include <type_traits>

#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
#define FONT_METRICS_WIDTH(fmt, ...) fmt.width(__VA_ARGS__)
#else
#define FONT_METRICS_WIDTH(fmt, ...) fmt.horizontalAdvance(__VA_ARGS__)
#endif

template <typename taIter>
struct EditingTagData {
    taIter tag_iter;
    QTextLayout *text_layout;
};
template <typename taIter>
EditingTagData(taIter, QTextLayout *) -> EditingTagData<taIter>;

/// @brief Computes bounding rectangles of the VisualTag.
class TagsRectsCalculator {
  public:
    TagsRectsCalculator() = delete;
    explicit TagsRectsCalculator(const QWidget &owner)
        : owner(owner)
    {
    }

    /// @brief Updates rect() field of the range of the VisualTag to fit owner's
    /// coordinate system.
    /// @note Empty tags are skip.
    template <typename taIter>
    void updateRects(taIter begin, taIter end) const
    {
        static_assert(
            std::is_same_v<typename std::iterator_traits<taIter>::value_type,
                           VisualTag>,
            "It requires an iterator over VisualTag.");
        auto const r = localContentRect();
        auto trackingOffset = r.topLeft();
        updateBoxedTags(trackingOffset, r.height(),
                        PredicateSkipIterator(begin, end, m_filter));
    }

    /// @brief Updates rect() field of the range of the VisualTag to fit owner's
    /// coordinate system.
    /// @param currently_edited describes and points to currently edited tag.
    /// @note Empty tags are skip.
    template <typename taIter>
    void updateRects(taIter begin, taIter end,
                     const EditingTagData<taIter> &currently_edited) const
    {
        static_assert(
            std::is_same_v<typename std::iterator_traits<taIter>::value_type,
                           VisualTag>,
            "It requires an iterator over VisualTag.");

        const bool is_edited = currently_edited.tag_iter != end &&
                               currently_edited.text_layout != nullptr;
        auto const r = localContentRect();
        auto trackingOffset = r.topLeft();

        if (!is_edited) {
            updateRects<taIter>(begin, end);
            return;
        }

        updateBoxedTags(
            trackingOffset, r.height(),
            PredicateSkipIterator(begin, currently_edited.tag_iter, m_filter));
        updateEditedTag(trackingOffset, r.height(), currently_edited);
        updateBoxedTags(
            trackingOffset, r.height(),
            PredicateSkipIterator(std::next(currently_edited.tag_iter, 1), end,
                                  m_filter));
    }

    /// @returns QRectF of "close" button for the tag, in local coordinate
    /// system (i.e. 0;0 matches it's corner).
    [[nodiscard]]
    static QRectF localCloseTagCrossRect(const QRectF &rect)
    {
        QRectF cross(QPointF{ 0.0f, 0.0f },
                     QSizeF{ tag_cross_width, tag_cross_width });
        cross.moveCenter(
            QPointF(rect.right() - tag_cross_width, rect.center().y()));
        return cross;
    }

    /// @returns QRect where text should be drawn in local coordinates.
    [[nodiscard]]
    QRect localContentRect() const
    {
        QStyleOptionFrame panel;
        initStyleOption(panel);
        QRect r = owner.style()->subElementRect(QStyle::SE_LineEditContents,
                                                &panel, &owner);
        r.adjust(left_text_margin, top_text_margin, -right_text_margin,
                 -bottom_text_margin);
        return r;
    }

    /// @brief Initializes @p option with values from owner + some
    /// additionals.
    void initStyleOption(QStyleOptionFrame &option) const
    {
        option.initFrom(&owner);
        option.rect = owner.contentsRect();
        option.lineWidth = owner.style()->pixelMetric(
            QStyle::PM_DefaultFrameWidth, &option, &owner);
        option.midLineWidth = 0;
        option.state |= QStyle::State_Sunken;
        option.features = QStyleOptionFrame::None;
    }

  protected:
    template <typename taIter, typename taPred>
    void updateBoxedTags(QPoint &trackingOffset, int height,
                         PredicateSkipIterator<taIter, taPred> range) const
    {
        for (; range.isValid(); ++range) {
            // calc text rect
            const auto i_width =
                FONT_METRICS_WIDTH(owner.fontMetrics(), range->text());
            QRect i_r(trackingOffset, QSize(i_width, height));
            i_r.translate(tag_inner_left_padding, 0);
            i_r.adjust(-tag_inner_left_padding, 0,
                       tag_inner_right_padding + tag_cross_spacing +
                           tag_cross_width,
                       0);
            range->setRect(i_r);
            trackingOffset.setX(i_r.right() + tag_spacing);
        }
    }

    template <typename taIter>
    void updateEditedTag(QPoint &trackingOffset, int height,
                         const EditingTagData<taIter> &edit_data) const
    {
        auto const w = FONT_METRICS_WIDTH(owner.fontMetrics(),
                                          edit_data.text_layout->text()) +
                       tag_inner_left_padding + tag_inner_right_padding;
        edit_data.tag_iter->setRect(QRect(trackingOffset, QSize(w, height)));
        trackingOffset += QPoint(w + tag_spacing, 0);
    }

  private:
    const QWidget &owner;

  public:
    static inline const auto m_filter = std::mem_fn(&VisualTag::isEmpty);

    static constexpr int tag_spacing = 3;
    static constexpr int tag_inner_left_padding = 3;
    static constexpr int tag_inner_right_padding = 4;
    static constexpr int tag_cross_spacing = 2;
    static constexpr int tag_cross_width = 5;

    static constexpr int top_text_margin = 1;
    static constexpr int bottom_text_margin = 1;
    static constexpr int left_text_margin = 1;
    static constexpr int right_text_margin = 1;
};

#undef FONT_METRICS_WIDTH
