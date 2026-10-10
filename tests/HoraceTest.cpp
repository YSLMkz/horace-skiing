#include "core/Horace.hpp"

#include <gtest/gtest.h>

using horace::Direction;
using horace::Horace;

TEST(HoraceTest, MovesOneStep) {
    Horace h(100, 100, 40, 40);
    h.move(Direction::Up, 800, 600);
    EXPECT_FLOAT_EQ(h.bounds().y, 60);
    h.move(Direction::Right, 800, 600);
    EXPECT_FLOAT_EQ(h.bounds().x, 140);
}

TEST(HoraceTest, CannotLeaveField) {
    Horace h(0, 0, 40, 40);
    h.move(Direction::Left, 800, 600);
    h.move(Direction::Up, 800, 600);
    EXPECT_FLOAT_EQ(h.bounds().x, 0);
    EXPECT_FLOAT_EQ(h.bounds().y, 0);
}

TEST(HoraceTest, ResetReturnsToStart) {
    Horace h(100, 200, 40, 40);
    h.move(Direction::Up, 800, 600);
    h.move(Direction::Left, 800, 600);
    h.resetToStart();
    EXPECT_FLOAT_EQ(h.bounds().x, 100);
    EXPECT_FLOAT_EQ(h.bounds().y, 200);
}
