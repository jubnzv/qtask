#pragma once

#include <iterator>
#include <type_traits>
#include <utility>

/// @brief Skips elements for which taPredicate(*it) evalutes to true.
template <typename taIter, typename taPredicate>
class PredicateSkipIterator {
  public:
    static_assert(std::is_invocable_r_v<bool, taPredicate,
                                        decltype(*std::declval<taIter>())>,
                  "Predicate must be callable with *It and return bool");

    using difference_type =
        typename std::iterator_traits<taIter>::difference_type;
    using value_type = typename std::iterator_traits<taIter>::value_type;
    using pointer = typename std::iterator_traits<taIter>::pointer;
    using reference = typename std::iterator_traits<taIter>::reference;
    using iterator_category = std::output_iterator_tag;

    PredicateSkipIterator() = delete;

    PredicateSkipIterator(taIter begin, taIter end, taPredicate predicate)
        : m_current(begin)
        , m_end(end)
        , m_predicate(std::move(predicate))
    {
        m_current = skipTillNextFalse(m_current);
    }

    PredicateSkipIterator &operator++()
    {
        if (isValid()) {
            m_current = skipTillNextFalse(++m_current);
        }
        return *this;
    }

    reference operator*() { return *m_current; }

    pointer operator->() { return &(*m_current); }

    taIter base_iterator() const { return m_current; }

    template <typename taPred>
    [[nodiscard]]
    bool operator!=(const PredicateSkipIterator<taIter, taPred> &rhs) const
    {
        return !(*this == rhs);
    }

    template <typename taPred>
    [[nodiscard]]
    bool operator==(const PredicateSkipIterator<taIter, taPred> &rhs) const
    {
        return m_current == rhs.m_current;
    }

    [[nodiscard]]
    bool isValid() const
    {
        return !(m_current == m_end);
    }

    [[nodiscard]]
    PredicateSkipIterator make_end() const
    {
        return { m_end, m_end, m_predicate };
    }

  private:
    template <typename I, typename P>
    friend class PredicateSkipIterator;

    taIter m_current;
    taIter m_end;
    taPredicate m_predicate;

    taIter skipTillNextFalse(taIter from) const
    {
        for (; from != m_end; ++from) {
            if (!m_predicate(*from)) {
                break;
            }
        }
        return from;
    }
};

template <class It, class Pred>
PredicateSkipIterator(It, It, Pred) -> PredicateSkipIterator<It, Pred>;
