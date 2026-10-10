#include "core/Game.hpp"

#include <gtest/gtest.h>

using horace::Car;
using horace::Game;
using horace::GameState;
using horace::Rect;

namespace {
// Машина, стоящая прямо на стартовой клетке Хораса
Car carOnHorace(const Game& game) {
    return Car(game.horace().bounds(), 0.f);
}
} // namespace

TEST(GameTest, StartsOnRoadWithStartMoney) {
    Game game;
    EXPECT_EQ(game.state(), GameState::Road);
    EXPECT_EQ(game.money(), Game::kStartMoney);
}

TEST(GameTest, NoCarsMeansNoDamage) {
    Game game;
    game.clearCars();
    game.update(1.f);
    EXPECT_EQ(game.money(), Game::kStartMoney);
}

TEST(GameTest, HitByCarCostsAmbulanceFee) {
    Game game;
    game.clearCars();
    game.addCar(carOnHorace(game));
    game.update(0.016f);
    EXPECT_EQ(game.money(), Game::kStartMoney - Game::kAmbulanceFee);
}

TEST(GameTest, HitSendsHoraceBackToStart) {
    Game game;
    game.clearCars();
    game.moveHorace(horace::Direction::Up);
    const Rect afterMove = game.horace().bounds();
    game.addCar(Car(afterMove, 0.f));
    game.update(0.016f);
    EXPECT_FLOAT_EQ(game.horace().bounds().y, Game::kFieldHeight - Game::kCellSize);
}

TEST(GameTest, GameOverWhenMoneyRunsOut) {
    Game game;
    game.clearCars();
    game.addCar(carOnHorace(game));
    for (int i = 0; i < Game::kStartMoney / Game::kAmbulanceFee; ++i) {
        game.update(0.016f);
    }
    EXPECT_EQ(game.state(), GameState::GameOver);
    EXPECT_EQ(game.money(), 0);
}

TEST(GameTest, CannotMoveAfterGameOver) {
    Game game;
    game.clearCars();
    game.addCar(carOnHorace(game));
    for (int i = 0; i < Game::kStartMoney / Game::kAmbulanceFee; ++i) {
        game.update(0.016f);
    }
    const Rect before = game.horace().bounds();
    game.moveHorace(horace::Direction::Up);
    EXPECT_FLOAT_EQ(game.horace().bounds().y, before.y);
}
