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

TEST(CarTest, ModelIsZeroByDefault) {
    Car car(Rect{0, 0, 80, 40}, 50.f);
    EXPECT_EQ(car.model(), 0);
}

TEST(CarTest, KeepsGivenModel) {
    Car car(Rect{0, 0, 80, 40}, 50.f, 3);
    EXPECT_EQ(car.model(), 3);
}

TEST(CarTest, NotChangingLaneByDefault) {
    Car car(Rect{0, 160, 80, 40}, 50.f);
    EXPECT_FALSE(car.isChangingLane());
    EXPECT_FLOAT_EQ(car.laneY(), 160);
}

TEST(CarTest, ChangesLaneSmoothly) {
    Car car(Rect{0, 160, 80, 40}, 50.f);
    car.changeLane(240, 70.f);
    EXPECT_TRUE(car.isChangingLane());
    car.update(0.1f, 800);
    EXPECT_GT(car.bounds().y, 160);   // уже едет вниз…
    EXPECT_LT(car.bounds().y, 240);   // …но ещё не доехала
    EXPECT_FLOAT_EQ(car.speed(), 50.f);  // и пока со старой скоростью
}

TEST(CarTest, TakesNewSpeedInNewLane) {
    Car car(Rect{0, 160, 80, 40}, 50.f);
    car.changeLane(240, 70.f);
    for (int i = 0; i < 100; ++i) car.update(0.05f, 800);
    EXPECT_FLOAT_EQ(car.speed(), 70.f);
}

TEST(CarTest, StopsExactlyInNewLane) {
    Car car(Rect{0, 240, 80, 40}, 50.f);
    car.changeLane(160, 50.f);  // вверх
    for (int i = 0; i < 100; ++i) car.update(0.05f, 800);
    EXPECT_FLOAT_EQ(car.bounds().y, 160);
    EXPECT_FALSE(car.isChangingLane());
}
