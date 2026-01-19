#include "tabular_stencil_base.hpp" // Твой хедер

#include <gtest/gtest.h>

#include <QString>
#include <QStringList>

namespace Test
{
struct TestTask {
    int id = -1;
    QString project;
    QString description;
};

// NOLINTNEXTLINE(cppcoreguidelines-virtual-class-destructor)
class TabularStencilSpy : public TabularStencilBase<TestTask> {
  public:
    // Making method to test as public
    using TabularStencilBase<TestTask>::parseConsoleOutput;
    using ColumnsSchema = TabularStencilBase<TestTask>::ColumnsSchema;
    using Mode = TabularStencilBase<TestTask>::ColumnDescriptor::LineMode;

    [[nodiscard]]
    const ColumnsSchema &getSchema() const override
    {
        static const ColumnsSchema s;
        return s;
    }

    [[nodiscard]]
    QStringList createCmdParameters(const TableStencil &) override // NOLINT
    {
        return {};
    }
};

class TabularParserTest : public ::testing::Test {
  protected:
    TabularStencilSpy spy;
    TabularStencilSpy::ColumnsSchema schema;
    using Mode = TabularStencilSpy::Mode;

    void SetUp() override
    {
        schema = { { "ID",
                     [](const QString &v, TestTask &t, Mode m) {
                         // If it is continuatiion we don't want to overwrite.
                         if (m == Mode::FirstLineOfNewRecord) {
                             t.id = v.toInt();
                         }
                     } },
                   { "Proj",
                     [](const QString &v, TestTask &t, Mode m) {
                         if (m == Mode::FirstLineOfNewRecord) {
                             t.project = v;
                         }
                     } },
                   { "Desc", [](const QString &v, TestTask &t, Mode) {
                        if (!t.description.isEmpty()) {
                            t.description += " ";
                        }
                        t.description += v;
                    } } };
    }
};

TEST_F(TabularParserTest, HandlesSimpleOutput)
{
    TableStencil stencil({ "ID", "Proj", "Desc" });
    ASSERT_TRUE(stencil.processLabelString("----|----|-----------"));

    const QStringList mockOutput = {
        "----|----|-----------", // Dividers (kRowIndexOfDividers)
        "ID  Proj Desc",         // Header (skipped by kHeadersSize)
        "1   Work Buy milk",     // Data 1
        "2   Home Clean room",   // Data 2
        "2 tasks"                // Footer (skipped by kFooterSize)
    };

    auto result = spy.parseConsoleOutput(schema, stencil, mockOutput);

    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].id, 1);
    EXPECT_EQ(result[0].project, "Work");
    EXPECT_EQ(result[1].description, "Clean room");
}

TEST_F(TabularParserTest, HandlesMultilineContinuation)
{
    TableStencil stencil({ "ID", "Proj", "Desc" });
    ASSERT_TRUE(stencil.processLabelString("----|----|-----------"));

    const QStringList mockOutput = { "----|----|-----------", // labels
                                     "ID  Proj Desc",         // header
                                     "1   Work Long task",
                                     "         description", // continuation 1
                                     "         part two", // continuation 2 more
                                     "2   Home Next task",

                                     "1 task" };

    auto result = spy.parseConsoleOutput(schema, stencil, mockOutput);

    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].id, 1);
    EXPECT_EQ(result[0].description, "Long task description part two");
    EXPECT_EQ(result[1].id, 2);
}

TEST_F(TabularParserTest, SkipsEmptyLines)
{
    TableStencil stencil({ "ID", "Proj", "Desc" });
    ASSERT_TRUE(stencil.processLabelString("----|----|-----------"));

    const QStringList mockOutput = { "----|----|-----------", // Markers
                                     "ID  Proj Desc",         // Header
                                     "1   Work Task 1",
                                     " ",    // Empty line to ignore
                                     "    ", // Spaces line to ignore
                                     "2   Home Task 2",
                                     "2 tasks" };

    auto result = spy.parseConsoleOutput(schema, stencil, mockOutput);

    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].id, 1);
    EXPECT_EQ(result[1].id, 2);
}

TEST_F(TabularParserTest, HandlesImpossibleContinuationGracefully)
{
    TableStencil stencil({ "ID", "Proj", "Desc" });
    ASSERT_TRUE(stencil.processLabelString("----|----|-----------"));

    // Ситуация: вывод начинается сразу с продолжения (ID пуст на первой строке
    // данных)
    const QStringList mockOutput = { "----|----|-----------", // labels
                                     "ID  Proj Desc",         // headers
                                     "         Broken task",  // broken output
                                     "2   Home Valid task", "1 task" };

    auto result = spy.parseConsoleOutput(schema, stencil, mockOutput);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].id, 2) << "First boken row had to be ignored.";
}
} // namespace Test
