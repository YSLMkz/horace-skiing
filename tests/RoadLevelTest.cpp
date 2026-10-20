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

// Ждёт, пока скорая уедет (не дольше 10 секунд). true — уехала.
bool waitForAmbulance(RoadLevel& road) {
    for (int i = 0; i < 1000; ++i) {
        if (road.update(0.01f) == RoadEvent::AmbulanceLeft) {
            return true;
        }
    }
    return false;
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

TEST(RoadLevelTest, HitCallsAmbulance) {
    RoadLevel road = emptyRoad();
    walk(road, Direction::Up, 3);
    putCarOnHorace(road);
    EXPECT_EQ(road.update(0.f), RoadEvent::HitByCar);
    EXPECT_TRUE(road.ambulance().isActive());
    EXPECT_TRUE(road.isHoraceKnockedDown());
}

TEST(RoadLevelTest, HoraceCannotMoveWhileAmbulanceComes) {
    RoadLevel road = emptyRoad();
    walk(road, Direction::Up, 3);
    putCarOnHorace(road);
    road.update(0.f);
    const float y = road.horace().bounds().y;
    road.moveHorace(Direction::Up);
    EXPECT_FLOAT_EQ(road.horace().bounds().y, y);
}

TEST(RoadLevelTest, TrafficStopsWhileAmbulanceComes) {
    RoadLevel road = emptyRoad();
    road.addCar(Car(horace::Rect{0, 4 * RoadLevel::kCell, 80, 40}, 100.f));  // едет по полосе
    walk(road, Direction::Up, 3);
    putCarOnHorace(road);
    road.update(0.f);
    const float carX = road.cars()[0].bounds().x;
    road.update(0.5f);
    EXPECT_FLOAT_EQ(road.cars()[0].bounds().x, carX);
}

TEST(RoadLevelTest, AmbulanceTakesHoraceAway) {
    RoadLevel road = emptyRoad();
    walk(road, Direction::Up, 3);
    putCarOnHorace(road);
    road.update(0.f);
    bool wasInside = false;
    for (int i = 0; i < 1000 && road.ambulance().isActive(); ++i) {
        road.update(0.01f);
        wasInside = wasInside || road.isHoraceInAmbulance();
    }
    EXPECT_TRUE(wasInside);
}

TEST(RoadLevelTest, AfterAmbulanceWithoutSkisHoraceIsAtBottom) {
    RoadLevel road = emptyRoad();
    walk(road, Direction::Up, 3);
    putCarOnHorace(road);
    road.update(0.f);
    ASSERT_TRUE(waitForAmbulance(road));
    EXPECT_FLOAT_EQ(road.horace().bounds().y, RoadLevel::kBottomY);
    EXPECT_FALSE(road.isHoraceKnockedDown());
}

TEST(RoadLevelTest, AfterAmbulanceWithSkisHoraceIsAtShopDoorWithSkis) {
    RoadLevel road = emptyRoad();
    road.giveSkis();
    walk(road, Direction::Down, 3);
    putCarOnHorace(road);
    road.update(0.f);
    ASSERT_TRUE(waitForAmbulance(road));
    EXPECT_FLOAT_EQ(road.horace().bounds().y, RoadLevel::kShopDoorY);
    EXPECT_TRUE(road.hasSkis());
}

TEST(RoadLevelTest, DefaultTrafficHasDifferentCarModels) {
    RoadLevel road;
    bool seen[RoadLevel::kCarModels] = {};
    for (const Car& car : road.cars()) {
        ASSERT_GE(car.model(), 0);
        ASSERT_LT(car.model(), RoadLevel::kCarModels);
        seen[car.model()] = true;
    }
    for (bool s : seen) EXPECT_TRUE(s);
}

TEST(RoadLevelTest, PlaceAtShopTakesSkisAway) {
    RoadLevel road = emptyRoad();
    road.giveSkis();
    road.placeAtShop();
    EXPECT_FALSE(road.hasSkis());
    EXPECT_FLOAT_EQ(road.horace().bounds().y, RoadLevel::kShopDoorY);
}

// ---------------------------------------------------------------------------
// Трафик как в оригинале: перестроения и рост числа машин
// ---------------------------------------------------------------------------

TEST(RoadLevelTest, LanesGoInPairsOfSameDirection) {
    for (int lane = 0; lane < RoadLevel::kLaneCount; ++lane) {
        const int partner = lane ^ 1;
        EXPECT_EQ(RoadLevel::laneSpeed(lane) > 0, RoadLevel::laneSpeed(partner) > 0);
    }
    EXPECT_GT(RoadLevel::laneSpeed(0), 0);  // верхние — вправо
    EXPECT_LT(RoadLevel::laneSpeed(3), 0);  // нижние — влево
}

TEST(RoadLevelTest, CarChangesToNeighbourLaneOfSameDirection) {
    RoadLevel road = emptyRoad();
    road.addCar(Car(horace::Rect{300, RoadLevel::laneY(0), 80, 40}, RoadLevel::laneSpeed(0)));
    ASSERT_TRUE(road.tryChangeLane(0));
    EXPECT_FLOAT_EQ(road.cars()[0].laneY(), RoadLevel::laneY(1));
    for (int i = 0; i < 100 && road.cars()[0].isChangingLane(); ++i) road.update(0.05f);
    EXPECT_FLOAT_EQ(road.cars()[0].bounds().y, RoadLevel::laneY(1));      // доехала
    EXPECT_FLOAT_EQ(road.cars()[0].speed(), RoadLevel::laneSpeed(1));     // и едет в темпе полосы
}

TEST(RoadLevelTest, NoLaneChangeIntoOccupiedSpot) {
    RoadLevel road = emptyRoad();
    road.addCar(Car(horace::Rect{300, RoadLevel::laneY(0), 80, 40}, RoadLevel::laneSpeed(0)));
    road.addCar(Car(horace::Rect{340, RoadLevel::laneY(1), 80, 40}, RoadLevel::laneSpeed(1)));  // рядом
    EXPECT_FALSE(road.tryChangeLane(0));
}

TEST(RoadLevelTest, NoLaneChangeWhileAlreadyChanging) {
    RoadLevel road = emptyRoad();
    road.addCar(Car(horace::Rect{300, RoadLevel::laneY(2), 80, 40}, RoadLevel::laneSpeed(2)));
    ASSERT_TRUE(road.tryChangeLane(0));
    EXPECT_FALSE(road.tryChangeLane(0));
}

TEST(RoadLevelTest, SpawnedCarEntersFromCorrectSide) {
    RoadLevel road = emptyRoad();
    ASSERT_TRUE(road.trySpawnCar());
    const Car& car = road.cars().back();
    if (car.speed() > 0) {
        EXPECT_LT(car.bounds().x, 0.f);              // едет вправо — появляется слева
    } else {
        EXPECT_GE(car.bounds().x, RoadLevel::kWidth);  // едет влево — справа
    }
}

TEST(RoadLevelTest, LanesNeverGetMoreThanMaxCars) {
    RoadLevel road = emptyRoad();
    for (int i = 0; i < 100; ++i) {
        road.trySpawnCar();
        road.update(0.5f);  // разъезжаются — освобождают въезд
    }
    for (int lane = 0; lane < RoadLevel::kLaneCount; ++lane) {
        int count = 0;
        for (const Car& car : road.cars()) {
            if (car.laneY() == RoadLevel::laneY(lane)) ++count;
        }
        EXPECT_LE(count, RoadLevel::kMaxCarsPerLane);
    }
}

TEST(RoadLevelTest, LingeringOnRoadBringsMoreCars) {
    RoadLevel road;  // обычный трафик; Хорас стоит внизу на тротуаре
    const size_t before = road.cars().size();
    for (int i = 0; i < 300; ++i) road.update(0.1f);  // 30 секунд
    EXPECT_GT(road.cars().size(), before);
    EXPECT_GT(road.crossingTime(), 29.f);
}

TEST(RoadLevelTest, NewCrossingResetsTraffic) {
    RoadLevel road;
    const size_t base = road.cars().size();
    for (int i = 0; i < 300; ++i) road.update(0.1f);
    road.placeAtShop();  // вернулись со склона
    EXPECT_EQ(road.cars().size(), base);
    EXPECT_FLOAT_EQ(road.crossingTime(), 0.f);
}

// Пять минут трафика с новыми машинами и перестроениями — машины ни разу не наезжают друг на друга
TEST(RoadLevelTest, TrafficNeverOverlaps) {
    RoadLevel road;  // Хорас на тротуаре, его не сбивают
    for (int frame = 0; frame < 5 * 60 * 60; ++frame) {
        road.update(1.f / 60);
        const auto& cars = road.cars();
        for (size_t i = 0; i < cars.size(); ++i) {
            for (size_t j = i + 1; j < cars.size(); ++j) {
                ASSERT_FALSE(cars[i].bounds().intersects(cars[j].bounds()))
                    << "frame " << frame << ": cars " << i << " and " << j;
            }
        }
    }
}
