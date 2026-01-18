#pragma once

#include <gtest/gtest.h>

#include <QCoreApplication>

#include <array>

class QtBaseTest : public ::testing::Test {
  protected:
    static void SetUpTestSuite()
    {
        static std::array<char, 5u> param = { 't', 'e', 's', 't', 0 };
        static std::array<char *, 2u> cmd = { param.data(), nullptr };
        static int count = 1;
        if (!qApp) {
            static const QCoreApplication app(count, cmd.data());
        }
    }
};
