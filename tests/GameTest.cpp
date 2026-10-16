#include "core/Game.hpp"

#include <gtest/gtest.h>

using horace::Car;
using horace::Direction;
using horace::Game;
using horace::GameState;
using horace::RoadLevel;

namespace {

Game gameWithEmptyRoad() {
    Game game;
    game.road().clearCars();
    return game;
}

void putCarOnHorace(Game& game) {
    game.road().addCar(Car(game.road().horace().bounds(), 0.f));
}

void walk(Game& game, Direction dir, int steps) {
    for (int i = 0; i < steps; ++i) {
        game.moveHorace(dir);
        game.update(0.f);
    }
}

// Крутит игру мелкими шагами, пока не изменятся деньги или не кончится игра
void runUntilMoneyChanges(Game& game) {
    const int before = game.money();
    for (int i = 0; i < 1000 && game.money() == before && game.state() != GameState::GameOver; ++i) {
        game.update(0.01f);
    }
}

// Машина стоит на старте — Хораса сбивают раз за разом, пока не кончатся деньги
void loseAllMoney(Game& game) {
    putCarOnHorace(game);
    for (int i = 0; i < 10000 && game.state() != GameState::GameOver; ++i) {
        game.update(0.01f);
    }
}

const int kRowsBottomToShop = 14;
const int kRowsDoorToBottom = 13;

// Довести игру до склона: дойти до лавки, взять лыжи, спуститься вниз
void goToSlope(Game& game) {
    walk(game, Direction::Up, kRowsBottomToShop);
    walk(game, Direction::Down, kRowsDoorToBottom);
}

} // namespace

TEST(GameTest, StartsOnRoadWithStartMoney) {
    Game game;
    EXPECT_EQ(game.state(), GameState::Road);
    EXPECT_EQ(game.money(), Game::kStartMoney);
    EXPECT_EQ(game.score(), 0);
}

TEST(GameTest, AmbulanceIsPaidOnlyAfterItLeaves) {
    Game game = gameWithEmptyRoad();
    putCarOnHorace(game);
    game.update(0.f);                       // сбили — скорая только выехала
    EXPECT_EQ(game.money(), Game::kStartMoney);
    runUntilMoneyChanges(game);             // скорая уехала — заплатили
    EXPECT_EQ(game.money(), Game::kStartMoney - Game::kAmbulanceFee);
    EXPECT_EQ(game.state(), GameState::Road);
}

TEST(GameTest, GameOverWhenMoneyRunsOut) {
    Game game = gameWithEmptyRoad();
    loseAllMoney(game);
    EXPECT_EQ(game.state(), GameState::GameOver);
    EXPECT_EQ(game.money(), 0);
}

TEST(GameTest, ReachingShopRentsSkis) {
    Game game = gameWithEmptyRoad();
    walk(game, Direction::Up, kRowsBottomToShop);
    EXPECT_EQ(game.money(), Game::kStartMoney - Game::kSkiPrice);
    EXPECT_TRUE(game.road().hasSkis());
}

TEST(GameTest, SkisAreRentedOnlyOnce) {
    Game game = gameWithEmptyRoad();
    walk(game, Direction::Up, kRowsBottomToShop);
    walk(game, Direction::Up, 3);  // топчемся у лавки
    EXPECT_EQ(game.money(), Game::kStartMoney - Game::kSkiPrice);
}

TEST(GameTest, CrossingBackWithSkisStartsSlope) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    EXPECT_EQ(game.state(), GameState::Slope);
}

TEST(GameTest, CannotWalkOnRoadDuringSlope) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    const float y = game.road().horace().bounds().y;
    game.moveHorace(Direction::Up);
    EXPECT_FLOAT_EQ(game.road().horace().bounds().y, y);
}

TEST(GameTest, AfterRunWithIntactSkisHoraceIsAtShopWithSkis) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    const int moneyBefore = game.money();
    game.finishRun(false);
    EXPECT_EQ(game.state(), GameState::Road);
    EXPECT_TRUE(game.road().hasSkis());
    EXPECT_FLOAT_EQ(game.road().horace().bounds().y, RoadLevel::kShopDoorY);
    EXPECT_EQ(game.money(), moneyBefore);
}

TEST(GameTest, AfterRunWithBrokenSkisMustRentAgain) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    const int moneyBefore = game.money();
    game.finishRun(true);
    EXPECT_FALSE(game.road().hasSkis());

    walk(game, Direction::Up, 1);  // шаг от двери в лавку
    EXPECT_TRUE(game.road().hasSkis());
    EXPECT_EQ(game.money(), moneyBefore - Game::kSkiPrice);
}

TEST(GameTest, FinishRunIgnoredWhenNotOnSlope) {
    Game game = gameWithEmptyRoad();
    game.finishRun(false);
    EXPECT_EQ(game.state(), GameState::Road);
    EXPECT_FALSE(game.road().hasSkis());
}

TEST(GameTest, CannotMoveAfterGameOver) {
    Game game = gameWithEmptyRoad();
    loseAllMoney(game);
    const float y = game.road().horace().bounds().y;
    game.moveHorace(Direction::Up);
    EXPECT_FLOAT_EQ(game.road().horace().bounds().y, y);
}

// ---------------------------------------------------------------------------
// Склон
// ---------------------------------------------------------------------------

namespace {

// Крутит игру, пока она на склоне (не дольше maxSeconds)
void rideWhileOnSlope(Game& game, float maxSeconds = 60.f) {
    for (float t = 0; t < maxSeconds && game.state() == GameState::Slope; t += 0.01f) {
        game.update(0.01f);
    }
}

float skierCenterX(const Game& game) {
    return game.slope().skier().x + game.slope().skier().width / 2.f;
}

} // namespace

TEST(GameTest, ReachingSlopeGeneratesCourse) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    EXPECT_EQ(static_cast<int>(game.slope().gates().size()), horace::SlopeLevel::kGateCount);
}

TEST(GameTest, PassingGateGivesPoints) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    game.slope().startEmptyRun(10000);
    game.slope().addGate(300, skierCenterX(game));
    for (int i = 0; i < 300; ++i) game.update(0.01f);
    EXPECT_EQ(game.score(), Game::kGatePoints);
}

TEST(GameTest, MissedGateGivesNoPoints) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    game.slope().startEmptyRun(10000);
    game.slope().addGate(300, skierCenterX(game) + 300);
    for (int i = 0; i < 300; ++i) game.update(0.01f);
    EXPECT_EQ(game.score(), 0);
}

TEST(GameTest, FinishGivesBonusAndKeepsSkis) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    const int moneyBefore = game.money();
    game.slope().startEmptyRun(1000);
    rideWhileOnSlope(game);
    EXPECT_EQ(game.state(), GameState::Road);
    EXPECT_EQ(game.score(), Game::kFinishBonus);
    EXPECT_TRUE(game.road().hasSkis());
    EXPECT_EQ(game.money(), moneyBefore);
    EXPECT_EQ(game.runsCompleted(), 1);
}

TEST(GameTest, BrokenSkisEndRunWithoutBonus) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    game.slope().startEmptyRun(10000);
    const float x = game.slope().skier().x;
    for (int i = 0; i < horace::SlopeLevel::kMaxDamage; ++i) {
        game.slope().addTree(x, 300 + i * 200.f);
    }
    rideWhileOnSlope(game);
    EXPECT_EQ(game.state(), GameState::Road);
    EXPECT_EQ(game.score(), 0);
    EXPECT_FALSE(game.road().hasSkis());
}

TEST(GameTest, EachRunHasNewCourse) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    const float firstGateX = game.slope().gates()[0].leftX;
    game.finishRun(false);
    walk(game, Direction::Down, kRowsDoorToBottom);  // лыжи целы — сразу вниз
    ASSERT_EQ(game.state(), GameState::Slope);
    EXPECT_NE(game.slope().gates()[0].leftX, firstGateX);
}
