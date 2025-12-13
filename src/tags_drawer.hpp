#pragma once

#include <QColor>
#include <QLine>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPointF>
#include <QStyle>
#include <QStyleOptionFrame>
#include <QVector>
#include <QtMinMax>
#include <QtNumeric>

#include "predicate_skip_iterator.hpp"
#include "tags_rects_calculator.h"

/// @brief Object used by TagsDrawer to track drawing state externaly.
/// @note You may want to use existing DrawerState when creating new TagsDrawer.
class IDrawerState {
  public:
    struct Selection {
        int select_start;
        int select_size;
    };

    [[nodiscard]]
    virtual int hScroll() const = 0;

    virtual void setHScroll(int value) = 0;
    [[nodiscard]]

    /// @returns Local cursor position inside currently edited tag.
    virtual int cursor() const = 0;

    [[nodiscard]]
    virtual int isDrawCursor() const = 0;

    [[nodiscard]]
    virtual Selection selection() const = 0;

    virtual ~IDrawerState() = default;
};

/// @brief Draws VisualTags objects based on computed before rectangles.
/// @note All tags at once must be drawn sequentally.
class TagsDrawer {
  public:
    explicit TagsDrawer(QWidget &owner)
        : params({ owner })
    {
    }

    /// @brief Draws all tags without editing state (all boxed).
    /// @note Empty tags are skip.
    template <typename taIter>
    void drawTags(const IDrawerState &state, taIter begin, taIter end) const
    {
        setupDrawing();
        drawBoxedTags(state, PredicateSkipIterator{
                                 begin, end, TagsRectsCalculator::m_filter });
    }

    /// @brief Draws all tags with editing state.
    /// @note Empty boxed tags (not edited) are skip.
    template <typename taIter>
    void drawTags(IDrawerState &state, taIter begin, taIter end,
                  const EditingTagData<taIter> &tag_data) const
    {
        if (tag_data.tag_iter == end) {
            drawTags(state, begin, end);
            return;
        }
        setupDrawing();
        const auto &r = tag_data.tag_iter->rect();
        const auto &txt_p =
            r.topLeft() +
            QPointF(
                TagsRectsCalculator::tag_inner_left_padding,
                ((r.height() - params.owner.fontMetrics().height()) / 2.0f));

        // Scroll.
        computeHorizontalScroll(r, state, begin, end, tag_data);

        // Boxed tags before (left) "editing" position.
        drawBoxedTags(state,
                      PredicateSkipIterator{ begin, tag_data.tag_iter,
                                             TagsRectsCalculator::m_filter });

        // Draw not terminated tag (currently edited).
        {
            const auto pos = txt_p - QPointF(state.hScroll(), 0);
            const auto formatting = this->formatting(state);
            tag_data.text_layout->draw(&params.painter, pos, formatting);

            // draw cursor
            if (state.isDrawCursor()) {
                tag_data.text_layout->drawCursor(&params.painter, pos,
                                                 state.cursor());
            }
        }
        // Boxed tags after (right) "editing" position.
        drawBoxedTags(
            state, PredicateSkipIterator{ std::next(tag_data.tag_iter, 1), end,
                                          TagsRectsCalculator::m_filter });
    }

    template <typename taIter>
    [[nodiscard]]
    static bool isPointOnCloseButton(const IDrawerState &state,
                                     const taIter iter, const QPoint &point)
    {
        return TagsRectsCalculator::localCloseTagCrossRect(iter->rect())
            .adjusted(-TagsRectsCalculator::tag_cross_spacing, 0, 0, 0)
            .translated(-state.hScroll(), 0)
            .contains(point);
    }

    /// @brief Iterates container @p cont and calls @p callback for each element
    /// which would be drawn (i.e. skipping empties) passing container's
    /// iterator to it.
    /// @tparam taContainer any iterable container.
    /// @tparam taCallback function which accepts taContainer::iterator. It
    /// should have bool result, if it returns false loop is break.
    /// @returns false if loop was break by @p callable or true otherwise.
    template <typename taContainer, typename taCallback>
    static bool ForEachDrawnTagIter(taContainer &&cont,
                                    const taCallback &callback)
    {
        auto &&c = std::forward<taContainer>(cont);
        PredicateSkipIterator iter(std::begin(c), std::end(c),
                                   TagsRectsCalculator::m_filter);
        for (; iter.isValid(); ++iter) {
            if (!callback(iter.base_iterator())) {
                return false;
            }
        }
        return true;
    }

  protected:
    struct DrawOutputParams {
        explicit DrawOutputParams(QWidget &owner)
            : owner(owner)
            , painter(&owner)
            , rects_calc(owner)
        {
        }
        QWidget &owner;
        mutable QPainter painter;
        TagsRectsCalculator rects_calc;
    };

    void setupDrawing() const
    {
        // opt
        auto opt = params.rects_calc.createStyleOption(params.owner);
        // draw frame
        params.owner.style()->drawPrimitive(QStyle::PE_PanelLineEdit, &opt,
                                            &params.painter, &params.owner);
        // clip
        params.painter.setClipRect(params.rects_calc.localContentRect());
    }

    /// @brief Draws "boxed" tag, i.e. which is not currently edited.
    template <typename taIter, typename taPredicateFilter>
    void
    drawBoxedTags(const IDrawerState &state,
                  PredicateSkipIterator<taIter, taPredicateFilter> range) const
    {
        const auto font_metrics = params.owner.fontMetrics();
        auto &painter = params.painter;

        for (; range.isValid(); ++range) {
            const QRect &i_r = range->rect().translated(-state.hScroll(), 0);
            const auto text_pos =
                i_r.topLeft() +
                QPointF(TagsRectsCalculator::tag_inner_left_padding,
                        font_metrics.ascent() +
                            ((i_r.height() - font_metrics.height()) / 2.0f));

            // drag tag rect
            QColor const blue(225, 236, 244);
            QPainterPath path;
            path.addRoundedRect(i_r, 4, 4);
            painter.fillPath(path, blue);

            // draw text
            painter.drawText(text_pos, range->text());

            // calc cross rect ("remove tag" button).
            auto const i_cross_r =
                TagsRectsCalculator::localCloseTagCrossRect(i_r);

            QPen pen = painter.pen();
            pen.setWidth(2);

            painter.save();
            painter.setPen(pen);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.drawLine(
                QLineF(i_cross_r.topLeft(), i_cross_r.bottomRight()));
            painter.drawLine(
                QLineF(i_cross_r.bottomLeft(), i_cross_r.topRight()));
            painter.restore();
        }
    }

    template <typename taIter>
    void computeHorizontalScroll(const QRect &forRect, IDrawerState &state,
                                 taIter begin, taIter end,
                                 const EditingTagData<taIter> &tag_data) const
    {
        if (begin == end) {
            state.setHScroll(0);
            return;
        }

        const float naturalWidth =
            std::prev(end, 1)->rect().right() - begin->rect().left();
        const auto xPosOfCursor =
            tag_data.text_layout->lineAt(0).cursorToX(state.cursor());

        const auto rect = params.rects_calc.localContentRect();
        const auto width_used = qRound(naturalWidth) + 1;
        const int cix = forRect.x() + qRound(xPosOfCursor);
        if (width_used <= rect.width()) {
            // text fit
            state.setHScroll(0);
        } else if (cix - state.hScroll() >= rect.width()) {
            // text doesn't fit, cursor is to the right of lineRect (scroll
            // right)
            state.setHScroll(cix - rect.width() + 1);
        } else if (cix - state.hScroll() < 0 && state.hScroll() < width_used) {
            // text doesn't fit, cursor is to the left of lineRect (scroll left)
            state.setHScroll(cix);
        } else if (width_used - state.hScroll() < rect.width()) {
            // text doesn't fit, text document is to the left of lineRect; align
            // right
            state.setHScroll(width_used - rect.width() + 1);
        } else {
            // in case the text is bigger than the lineedit, the hscroll can
            // never be negative
            state.setHScroll(qMax(0, state.hScroll()));
        }
    }

    QVector<QTextLayout::FormatRange>
    formatting(const IDrawerState &state) const
    {
        const auto sel = state.selection();

        if (sel.select_size == 0) {
            return {};
        }

        QTextLayout::FormatRange selection;
        selection.start = sel.select_start;
        selection.length = sel.select_size;
        selection.format.setBackground(
            params.owner.palette().brush(QPalette::Highlight));
        selection.format.setForeground(
            params.owner.palette().brush(QPalette::HighlightedText));
        return { selection };
    }

  private:
    DrawOutputParams params;
};
