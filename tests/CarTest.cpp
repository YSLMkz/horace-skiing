#include "core/Car.hpp"

#include <gtest/gtest.h>

using horace::Car;
using horace::Rect;

TEST(CarTest, MovesBySpeedTimesDt) {
    Car car(Rect{100, 0, 80, 40}, 50.f);
    car.update(2.f, 800);
    EXPECT_FLOAT_EQ(car.bounds().x, 200);
}

TEST(CarTest, WrapsFromRightToLeft) {
    Car car(Rect{790, 0, 80, 40}, 100.f);
    car.update(1.f, 800);  // x = 890 > 800 — уехала
    EXPECT_FLOAT_EQ(car.bounds().x, -80);
}

TEST(CarTest, WrapsFromLeftToRight) {
    Car car(Rect{-70, 0, 80, 40}, -100.f);
    car.update(1.f, 800);  // правый край < 0 — уехала
    EXPECT_FLOAT_EQ(car.bounds().x, 800);
}
