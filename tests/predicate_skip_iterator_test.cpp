#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "predicate_skip_iterator.hpp"

using PredicateSkipIteratorTest = ::testing::Test;

struct Tag {
    std::string text;
};

TEST(PredicateSkipIteratorTest, BasicFiltering)
{
    const std::vector<Tag> tags = { { "a" }, { "" }, { "b" }, { "" }, { "c" } };

    static const auto pred = [](const Tag &t) { return t.text.empty(); };
    PredicateSkipIterator it(tags.begin(), tags.end(), pred);
    const auto end = it.make_end();

    std::vector<std::string> collected;
    for (; it != end; ++it) {
        collected.push_back(it->text);
    }

    const std::vector<std::string> expected = { "a", "b", "c" };
    EXPECT_EQ(collected, expected);
}

TEST(PredicateSkipIteratorTest, EmptyContainer)
{
    std::vector<Tag> empty;

    static const auto pred = [](const Tag &t) { return t.text.empty(); };
    const PredicateSkipIterator it(empty.begin(), empty.end(), pred);
    const auto end = it.make_end();

    EXPECT_FALSE(it.isValid());
    EXPECT_EQ(it, end);
}

TEST(PredicateSkipIteratorTest, AllSkipped)
{
    const std::vector<Tag> tags = { { "a" }, { "b" }, { "c" } };
    auto pred = [](const Tag &) { return true; };

    const PredicateSkipIterator it(tags.begin(), tags.end(), pred);
    const auto end = it.make_end();

    EXPECT_FALSE(it.isValid());
    EXPECT_EQ(it, end);
}

TEST(PredicateSkipIteratorTest, StatefulPredicate)
{
    const std::vector<Tag> tags = { { "x" }, { "y" }, { "z" } };
    int counter = 0;
    auto pred = [&counter](const Tag &) {
        ++counter;
        return counter % 2 == 0;
    };

    PredicateSkipIterator it(tags.begin(), tags.end(), pred);
    const auto end = it.make_end();

    std::vector<std::string> collected;
    for (; it != end; ++it) {
        collected.push_back(it->text);
    }

    EXPECT_EQ(collected.size(), 2);
}

TEST(PredicateSkipIteratorTest, SingleElementValid)
{
    const std::vector<Tag> tags = { { "only" } };
    auto pred = [](const Tag & /*t*/) { return false; };

    PredicateSkipIterator it(tags.begin(), tags.end(), pred);
    const auto end = it.make_end();

    EXPECT_TRUE(it.isValid());
    EXPECT_EQ(it->text, "only");

    ++it;
    EXPECT_FALSE(it.isValid());
    EXPECT_EQ(it, end);
}

TEST(PredicateSkipIteratorTest, SingleElementSkipped)
{
    const std::vector<Tag> tags = { { "skip" } };
    auto pred = [](const Tag & /*t*/) { return true; };

    const PredicateSkipIterator it(tags.begin(), tags.end(), pred);
    const auto end = it.make_end();

    EXPECT_FALSE(it.isValid());
    EXPECT_EQ(it, end);
}

TEST(PredicateSkipIteratorTest, IncrementEndIteratorSafe)
{
    const std::vector<Tag> tags = { { "a" }, { "b" } };
    static const auto pred = [](const Tag & /*t*/) { return false; };

    const PredicateSkipIterator it(tags.begin(), tags.end(), pred);
    const auto end = it.make_end();

    EXPECT_TRUE(it.isValid());
    EXPECT_FALSE(end.isValid());

    auto copy = end;
    ++copy;
    EXPECT_FALSE(copy.isValid()) << "++end must be safe.";

    ++copy;
    EXPECT_FALSE(copy.isValid()) << "++end must be safe.";

    EXPECT_NE(it, end);
    EXPECT_NE(it, copy);
    EXPECT_EQ(end, copy);
}
