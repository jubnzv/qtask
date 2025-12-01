#include "taskproperty.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <tuple>

namespace Test
{
using TaskPropertyTest = ::testing::Test;

template <typename taType>
QStringList getValidStrings(const TaskProperty<taType> &prop)
{
    QStringList res;
    EXPECT_NO_THROW(res = prop.getStringsForCmd());
    return res;
}

TEST_F(TaskPropertyTest, ThrowsIfQStringArgNotSupported)
{
    enum class UnsupportedEnum : std::uint8_t { Value1, Value2 };
    const TaskProperty<UnsupportedEnum> prop("value: %1",
                                             UnsupportedEnum::Value1);
    EXPECT_THROW(std::ignore = prop.getStringsForCmd(), std::runtime_error);
}

TEST_F(TaskPropertyTest, WorksIfQStringArgSupported)
{
    const TaskProperty<int> prop("value: %1", 2);
    const auto list = getValidStrings(prop);
    EXPECT_EQ(list.size(), 1u);
    EXPECT_EQ(list.first(), QString("value: 2"));
}

TEST_F(TaskPropertyTest, WorksWithFormatterToQString)
{
    static constexpr int kExpectedValue = 71;

    const auto formatter = [](int val) -> QString {
        EXPECT_EQ(val, kExpectedValue);
        return QString("v:%1").arg(val);
    };
    const TaskProperty<int> prop(formatter, kExpectedValue);
    const auto list = getValidStrings(prop);
    EXPECT_EQ(list.size(), 1u);
    EXPECT_EQ(list.first(), QString("v:71"));
}

TEST_F(TaskPropertyTest, WorksWithFormatterToQStringList)
{
    static constexpr int kExpectedValue = 71;
    static const QStringList fmtList = { "s1", "s2" };

    const auto formatter = [](int val) -> QStringList {
        EXPECT_EQ(val, kExpectedValue);
        return fmtList;
    };
    const TaskProperty<int> prop(formatter, kExpectedValue);
    const auto list = getValidStrings(prop);
    EXPECT_EQ(list, fmtList);
}

TEST_F(TaskPropertyTest, CallingModifySetsFlag)
{
    TaskProperty<int> prop("value: %1", 2);
    EXPECT_FALSE(prop.isModified());
    prop.modify([](auto &) {});
    EXPECT_TRUE(prop.isModified());
}

TEST_F(TaskPropertyTest, ValueCanBeSetAndModificationReset)
{
    TaskProperty<int> prop("value: %1", 2);
    EXPECT_FALSE(prop.isModified());
    EXPECT_EQ(prop.get(), 2);

    prop = 3;
    EXPECT_TRUE(prop.isModified());
    EXPECT_EQ(prop.get(), 3);

    prop.setNotModified();
    EXPECT_FALSE(prop.isModified());
    EXPECT_EQ(prop.get(), 3);
}

} // namespace Test
