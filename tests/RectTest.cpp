#include "core/Rect.hpp"

#include <gtest/gtest.h>

using horace::Rect;

TEST(RectTest, OverlappingRectsIntersect) {
    Rect a{0, 0, 10, 10};
    Rect b{5, 5, 10, 10};
    EXPECT_TRUE(a.intersects(b));
    EXPECT_TRUE(b.intersects(a));
}

TEST(RectTest, SeparateRectsDoNotIntersect) {
    Rect a{0, 0, 10, 10};
    Rect b{20, 20, 5, 5};
    EXPECT_FALSE(a.intersects(b));
}

TEST(RectTest, TouchingEdgesDoNotIntersect) {
    Rect a{0, 0, 10, 10};
    Rect b{10, 0, 10, 10};
    EXPECT_FALSE(a.intersects(b));
}
