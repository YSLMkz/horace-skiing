#include "core/Horace.hpp"

#include <gtest/gtest.h>

using horace::Direction;
using horace::Horace;

TEST(HoraceTest, MovesOneStep) {
    Horace h(40, 40);
    h.setPosition(100, 100);
    h.move(Direction::Up, 800, 600);
    EXPECT_FLOAT_EQ(h.bounds().y, 60);
    h.move(Direction::Right, 800, 600);
    EXPECT_FLOAT_EQ(h.bounds().x, 140);
}

TEST(HoraceTest, CannotLeaveField) {
    Horace h(40, 40);
    h.setPosition(0, 0);
    h.move(Direction::Left, 800, 600);
    h.move(Direction::Up, 800, 600);
    EXPECT_FLOAT_EQ(h.bounds().x, 0);
    EXPECT_FLOAT_EQ(h.bounds().y, 0);
}

TEST(HoraceTest, CannotLeaveFieldBottomRight) {
    Horace h(40, 40);
    h.setPosition(760, 560);
    h.move(Direction::Right, 800, 600);
    h.move(Direction::Down, 800, 600);
    EXPECT_FLOAT_EQ(h.bounds().x, 760);
    EXPECT_FLOAT_EQ(h.bounds().y, 560);
}

TEST(HoraceTest, SetPositionMovesHorace) {
    Horace h(40, 40);
    h.setPosition(100, 200);
    EXPECT_FLOAT_EQ(h.bounds().x, 100);
    EXPECT_FLOAT_EQ(h.bounds().y, 200);
}
