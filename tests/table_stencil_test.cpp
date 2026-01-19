#include "task_table_stencil.hpp"

#include <QString>
#include <QStringList>
#include <qtypes.h>

#include <stdexcept>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace Test
{
class TableStencilTest : public ::testing::Test {
  protected:
    std::vector<std::pair<QString, QString>> parse(TableStencil &q,
                                                   const QString &line)
    {
        std::vector<std::pair<QString, QString>> res;
        q.forEachColumn(line, [&](qsizetype index, const QString &name,
                                  const QString &val) {
            EXPECT_EQ(index, res.size());
            res.emplace_back(name, val);
        });
        return res;
    }
};

TEST_F(TableStencilTest, FullFlowTest)
{
    const QStringList headers = { "ID", "Proj", "Date", "Desc" }; // NOLINT
    TableStencil query(headers);

    const QString labelLine = "   |      |          |";
    const QString line_____ = "5  Living 2026-01-28 Salary"; // NOLINT

    ASSERT_TRUE(query.processLabelString(labelLine));

    auto res = parse(query, line_____);

    ASSERT_EQ(res.size(), 4);
    EXPECT_EQ(res[0].second, "5");
    EXPECT_EQ(res[1].second, "Living");
    EXPECT_EQ(res[2].second, "2026-01-28");
    EXPECT_EQ(res[3].second, "Salary");
}

TEST_F(TableStencilTest, Multiline)
{
    const QStringList headers = { "ID", "Proj", "Date", "Desc" };
    TableStencil query(headers);

    const QString labelLine = "   |      |          |";
    const QString line_____ = "                     Salary"; // NOLINT

    ASSERT_TRUE(query.processLabelString(labelLine));

    auto res = parse(query, line_____);

    ASSERT_EQ(res.size(), 4);
    EXPECT_EQ(res[0].second, "");
    EXPECT_EQ(res[1].second, "");
    EXPECT_EQ(res[2].second, "");
    EXPECT_EQ(res[3].second, "Salary");
}

TEST_F(TableStencilTest, OverflowData)
{
    // "ID|Project"
    TableStencil query({ "ID", "Project" });
    const QString labelLine = "   |";
    const QString line_____ = "100MyProject"; // NOLINT
    ASSERT_TRUE(query.processLabelString(labelLine));
    auto res = parse(query, line_____);
    EXPECT_EQ(res[0].second, "100");
    EXPECT_EQ(res[1].second, "MyProject");
}

TEST_F(TableStencilTest, TrailingEmpty)
{
    TableStencil query({ "ID", "Proj", "Desc" });
    const QString labelLine = "   |      |";
    const QString line_____ = "1  Home"; // NOLINT
    ASSERT_TRUE(query.processLabelString(labelLine));

    auto res = parse(query, line_____);
    ASSERT_EQ(res.size(), 3);
    EXPECT_EQ(res[0].second, "1");
    EXPECT_EQ(res[1].second, "Home");
    EXPECT_EQ(res[2].second, ""); // Desc
}

TEST_F(TableStencilTest, UnicodeHandling)
{
    TableStencil query({ "ID", "Desc" });
    const QString labelLine = "   |";
    const QString line_____ = "1  Купить хлеб"; // NOLINT
    ASSERT_TRUE(query.processLabelString(labelLine));

    auto res = parse(query, line_____);
    EXPECT_EQ(res[0].second, "1");
    EXPECT_EQ(res[1].second, "Купить хлеб");
}

TEST_F(TableStencilTest, ThrowsOnUninitialized)
{
    const TableStencil query({ "A", "B" });
    static const auto handler = [](qsizetype, const QString &,
                                   const QString &) {};
    EXPECT_THROW(query.forEachColumn("some data", handler), std::runtime_error);
}
} // namespace Test
