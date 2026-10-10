#include "core/RoadLevel.hpp"

#include <gtest/gtest.h>

using horace::Car;
using horace::Direction;
using horace::RoadEvent;
using horace::RoadLevel;

namespace {

// Пустая дорога без машин — чтобы проверять ходьбу без случайных столкновений
RoadLevel emptyRoad() {
    RoadLevel road;
    road.clearCars();
    return road;
}

// Ставит неподвижную машину прямо на Хораса
void putCarOnHorace(RoadLevel& road) {
    road.addCar(Car(road.horace().bounds(), 0.f));
}

// Идёт в одном направлении steps шагов, возвращает событие после последнего шага
RoadEvent walk(RoadLevel& road, Direction dir, int steps) {
    RoadEvent last = RoadEvent::None;
    for (int i = 0; i < steps; ++i) {
        road.moveHorace(dir);
        last = road.update(0.f);
    }
    return last;
}

const int kRowsBottomToShop = static_cast<int>(RoadLevel::kBottomY / RoadLevel::kCell);       // 14
const int kRowsDoorToBottom = static_cast<int>((RoadLevel::kBottomY - RoadLevel::kShopDoorY) / RoadLevel::kCell); // 13

} // namespace

TEST(RoadLevelTest, StartsAtBottomWithoutSkis) {
    RoadLevel road;
    EXPECT_FALSE(road.hasSkis());
    EXPECT_FLOAT_EQ(road.horace().bounds().y, RoadLevel::kBottomY);
}

TEST(RoadLevelTest, DefaultTrafficDoesNotHitHoraceAtStart) {
    RoadLevel road;
    EXPECT_EQ(road.update(0.f), RoadEvent::None);
}

TEST(RoadLevelTest, WalkingUpReachesShop) {
    RoadLevel road = emptyRoad();
    EXPECT_EQ(walk(road, Direction::Up, kRowsBottomToShop), RoadEvent::ReachedShop);
}

TEST(RoadLevelTest, TopRowAwayFromShopIsNotShop) {
    RoadLevel road = emptyRoad();
    walk(road, Direction::Left, 5);
    EXPECT_EQ(walk(road, Direction::Up, kRowsBottomToShop), RoadEvent::None);
}

TEST(RoadLevelTest, BottomWithoutSkisIsNotSlope) {
    RoadLevel road = emptyRoad();
    EXPECT_EQ(road.update(0.f), RoadEvent::None);  // стоим внизу без лыж
}

TEST(RoadLevelTest, WithSkisWalkingDownReachesSlope) {
    RoadLevel road = emptyRoad();
    road.giveSkis();
    EXPECT_EQ(walk(road, Direction::Down, kRowsDoorToBottom), RoadEvent::ReachedSlope);
}

TEST(RoadLevelTest, HitWithoutSkisSendsHoraceToBottom) {
    RoadLevel road = emptyRoad();
    walk(road, Direction::Up, 3);
    putCarOnHorace(road);
    EXPECT_EQ(road.update(0.f), RoadEvent::HitByCar);
    EXPECT_FLOAT_EQ(road.horace().bounds().y, RoadLevel::kBottomY);
}

TEST(RoadLevelTest, HitWithSkisSendsHoraceToShopDoorWithSkis) {
    RoadLevel road = emptyRoad();
    road.giveSkis();
    walk(road, Direction::Down, 3);
    putCarOnHorace(road);
    EXPECT_EQ(road.update(0.f), RoadEvent::HitByCar);
    EXPECT_FLOAT_EQ(road.horace().bounds().y, RoadLevel::kShopDoorY);
    EXPECT_TRUE(road.hasSkis());
}

TEST(RoadLevelTest, PlaceAtShopTakesSkisAway) {
    RoadLevel road = emptyRoad();
    road.giveSkis();
    road.placeAtShop();
    EXPECT_FALSE(road.hasSkis());
    EXPECT_FLOAT_EQ(road.horace().bounds().y, RoadLevel::kShopDoorY);
}
