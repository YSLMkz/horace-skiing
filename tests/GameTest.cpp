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

TEST(GameTest, HitByCarCostsAmbulanceFee) {
    Game game = gameWithEmptyRoad();
    putCarOnHorace(game);
    game.update(0.f);
    EXPECT_EQ(game.money(), Game::kStartMoney - Game::kAmbulanceFee);
    EXPECT_EQ(game.state(), GameState::Road);
}

TEST(GameTest, GameOverWhenMoneyRunsOut) {
    Game game = gameWithEmptyRoad();
    putCarOnHorace(game);  // машина стоит на старте — сбивает каждый кадр
    for (int i = 0; i < Game::kStartMoney / Game::kAmbulanceFee; ++i) {
        game.update(0.f);
    }
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
    putCarOnHorace(game);
    for (int i = 0; i < Game::kStartMoney / Game::kAmbulanceFee; ++i) {
        game.update(0.f);
    }
    const float y = game.road().horace().bounds().y;
    game.moveHorace(Direction::Up);
    EXPECT_FLOAT_EQ(game.road().horace().bounds().y, y);
}
