#include "core/Game.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

using horace::Car;
using horace::Direction;
using horace::Game;
using horace::GameState;
using horace::RoadLevel;

namespace {

// Новая игра, уже начатая из меню (Enter), с пустой дорогой
Game gameWithEmptyRoad() {
    Game game;
    game.confirm();
    game.road().clearCars();
    game.takeEvents();
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

TEST(GameTest, StartsInMenu) {
    Game game;
    EXPECT_EQ(game.state(), GameState::Menu);
    EXPECT_EQ(game.money(), Game::kStartMoney);
    EXPECT_EQ(game.score(), 0);
}

TEST(GameTest, EnterInMenuStartsOnRoad) {
    Game game;
    game.confirm();
    EXPECT_EQ(game.state(), GameState::Road);
    EXPECT_FLOAT_EQ(game.road().horace().bounds().y, RoadLevel::kBottomY);
}

TEST(GameTest, CannotWalkInMenu) {
    Game game;
    const float y = game.road().horace().bounds().y;
    game.moveHorace(Direction::Up);
    EXPECT_FLOAT_EQ(game.road().horace().bounds().y, y);
}

TEST(GameTest, TrafficMovesInMenuBackground) {
    Game game;
    const float x = game.road().cars()[0].bounds().x;
    game.update(0.5f);
    EXPECT_NE(game.road().cars()[0].bounds().x, x);
    EXPECT_EQ(game.state(), GameState::Menu);
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
    EXPECT_EQ(game.state(), GameState::RunResults);  // сначала итоги
    game.confirm();
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
    game.confirm();
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
    EXPECT_EQ(game.state(), GameState::RunResults);
    game.confirm();
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
    EXPECT_EQ(game.state(), GameState::RunResults);
    game.confirm();
    EXPECT_EQ(game.score(), 0);
    EXPECT_FALSE(game.road().hasSkis());
}

TEST(GameTest, EachRunHasNewCourse) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    const float firstGateX = game.slope().gates()[0].leftX;
    game.finishRun(false);
    game.confirm();           // экран итогов -> дорога
    game.road().clearCars();  // после заезда трафик на дороге начинается заново — убираем
    walk(game, Direction::Down, kRowsDoorToBottom);  // лыжи целы — сразу вниз
    ASSERT_EQ(game.state(), GameState::Slope);
    EXPECT_NE(game.slope().gates()[0].leftX, firstGateX);
}

// ---------------------------------------------------------------------------
// События (по ним app играет звуки)
// ---------------------------------------------------------------------------

namespace {

using horace::GameEvent;

bool has(const std::vector<GameEvent>& events, GameEvent e) {
    return std::find(events.begin(), events.end(), e) != events.end();
}

// Крутит игру и собирает все события, пока не появится нужное (не дольше maxSeconds)
std::vector<GameEvent> collectUntil(Game& game, GameEvent wanted, float maxSeconds = 60.f) {
    std::vector<GameEvent> all;
    for (float t = 0; t < maxSeconds && !has(all, wanted); t += 0.01f) {
        game.update(0.01f);
        for (GameEvent e : game.takeEvents()) all.push_back(e);
    }
    return all;
}

} // namespace

TEST(GameEventsTest, StepMakesEvent) {
    Game game = gameWithEmptyRoad();
    game.moveHorace(Direction::Up);
    EXPECT_EQ(game.takeEvents(), std::vector<GameEvent>{GameEvent::Step});
}

TEST(GameEventsTest, NoStepEventWhenBlockedByEdge) {
    Game game = gameWithEmptyRoad();
    game.moveHorace(Direction::Down);  // уже в самом низу — не сдвинется
    EXPECT_TRUE(game.takeEvents().empty());
}

TEST(GameEventsTest, TakeEventsClearsList) {
    Game game = gameWithEmptyRoad();
    game.moveHorace(Direction::Up);
    game.takeEvents();
    EXPECT_TRUE(game.takeEvents().empty());
}

TEST(GameEventsTest, CrashMakesEvent) {
    Game game = gameWithEmptyRoad();
    putCarOnHorace(game);
    game.update(0.f);
    EXPECT_TRUE(has(game.takeEvents(), GameEvent::Crash));
}

TEST(GameEventsTest, RentingSkisMakesEvent) {
    Game game = gameWithEmptyRoad();
    walk(game, Direction::Up, kRowsBottomToShop);
    EXPECT_TRUE(has(game.takeEvents(), GameEvent::SkisRented));
}

TEST(GameEventsTest, GatesMakeEvents) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    game.slope().startEmptyRun(10000);
    game.slope().addGate(300, skierCenterX(game));        // по курсу
    game.slope().addGate(600, skierCenterX(game) + 300);  // сбоку
    game.takeEvents();
    const auto events = collectUntil(game, GameEvent::GateMissed);
    EXPECT_TRUE(has(events, GameEvent::GatePassed));
    EXPECT_TRUE(has(events, GameEvent::GateMissed));
}

TEST(GameEventsTest, TreeAndBrokenSkisMakeEvents) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    game.slope().startEmptyRun(10000);
    const float x = game.slope().skier().x;
    for (int i = 0; i < horace::SlopeLevel::kMaxDamage; ++i) {
        game.slope().addTree(x, 300 + i * 200.f);
    }
    const auto events = collectUntil(game, GameEvent::SkisBroken);
    EXPECT_TRUE(has(events, GameEvent::TreeHit));
    EXPECT_TRUE(has(events, GameEvent::SkisBroken));
}

TEST(GameEventsTest, FinishMakesEvent) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    game.slope().startEmptyRun(1000);
    EXPECT_TRUE(has(collectUntil(game, GameEvent::Finished), GameEvent::Finished));
}

TEST(GameEventsTest, GameOverMakesEvent) {
    Game game = gameWithEmptyRoad();
    putCarOnHorace(game);
    EXPECT_TRUE(has(collectUntil(game, GameEvent::GameOver), GameEvent::GameOver));
}

// ---------------------------------------------------------------------------
// Правила оригинала: штраф за ворота, деньги за очки, пауза
// ---------------------------------------------------------------------------

namespace {

// Склон с count воротами подряд точно по курсу (через каждые 100 px)
void slopeWithGatesAhead(Game& game, int count, float firstY = 300) {
    game.slope().startEmptyRun(firstY + count * 100.f + 100000.f);
    for (int i = 0; i < count; ++i) {
        game.slope().addGate(firstY + i * 100.f, skierCenterX(game));
    }
}

void rideSeconds(Game& game, float seconds) {
    for (float t = 0; t < seconds; t += 0.01f) game.update(0.01f);
}

} // namespace

TEST(GameRulesTest, MissedGateCostsPoints) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    slopeWithGatesAhead(game, 2);
    game.slope().addGate(600, skierCenterX(game) + 300);  // третьи — сбоку
    rideSeconds(game, 3.f);
    EXPECT_EQ(game.score(), 2 * Game::kGatePoints - Game::kGatePenalty);
}

TEST(GameRulesTest, ScoreNeverGoesBelowZero) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    game.slope().startEmptyRun(10000);
    game.slope().addGate(300, skierCenterX(game) + 300);
    rideSeconds(game, 2.f);
    EXPECT_EQ(game.score(), 0);
}

TEST(GameRulesTest, Every1000PointsGivesCash) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    const int moneyBefore = game.money();
    const int gates = Game::kPointsForCash / Game::kGatePoints;  // ровно на 1000 очков
    slopeWithGatesAhead(game, gates);
    game.takeEvents();
    const auto events = collectUntil(game, GameEvent::BonusMoney);
    EXPECT_EQ(game.score(), Game::kPointsForCash);
    EXPECT_EQ(game.money(), moneyBefore + Game::kCashBonus);
    EXPECT_EQ(game.nextCashAt(), 2 * Game::kPointsForCash);
    EXPECT_TRUE(has(events, GameEvent::BonusMoney));
}

TEST(GameRulesTest, SameThousandIsNotPaidTwice) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    const int moneyBefore = game.money();
    const int gates = Game::kPointsForCash / Game::kGatePoints;
    slopeWithGatesAhead(game, gates);
    const float afterLast = 300 + gates * 100.f;
    game.slope().addGate(afterLast, skierCenterX(game) + 300);        // пропуск: 1000 -> 990
    game.slope().addGate(afterLast + 100, skierCenterX(game));        // снова 1000
    rideSeconds(game, (afterLast + 300) / horace::SlopeLevel::kDownSpeed);
    EXPECT_EQ(game.score(), Game::kPointsForCash);
    EXPECT_EQ(game.money(), moneyBefore + Game::kCashBonus);  // заплатили один раз
}

TEST(GameRulesTest, PauseFreezesGame) {
    Game game = gameWithEmptyRoad();
    game.road().addCar(Car(horace::Rect{0, RoadLevel::laneY(0), 80, 40}, 100.f));
    game.togglePause();
    EXPECT_TRUE(game.isPaused());
    game.update(1.f);
    EXPECT_FLOAT_EQ(game.road().cars()[0].bounds().x, 0.f);  // машина стоит
    const float y = game.road().horace().bounds().y;
    game.moveHorace(Direction::Up);
    EXPECT_FLOAT_EQ(game.road().horace().bounds().y, y);     // Хорас стоит
}

TEST(GameRulesTest, UnpauseContinues) {
    Game game = gameWithEmptyRoad();
    game.road().addCar(Car(horace::Rect{0, RoadLevel::laneY(0), 80, 40}, 100.f));
    game.togglePause();
    game.togglePause();
    game.update(1.f);
    EXPECT_GT(game.road().cars()[0].bounds().x, 0.f);
}

TEST(GameRulesTest, CannotPauseAfterGameOver) {
    Game game = gameWithEmptyRoad();
    loseAllMoney(game);
    game.togglePause();
    EXPECT_FALSE(game.isPaused());
}

// ---------------------------------------------------------------------------
// Итоги заезда и конец игры
// ---------------------------------------------------------------------------

TEST(RunResultsTest, StatsAreCounted) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    game.slope().startEmptyRun(1500);
    const float cx = skierCenterX(game);
    game.slope().addGate(300, cx);          // пройдём
    game.slope().addGate(500, cx);          // пройдём
    game.slope().addGate(700, cx + 300);    // пропустим
    game.slope().addTree(game.slope().skier().x, 900);  // врежемся
    rideWhileOnSlope(game);
    ASSERT_EQ(game.state(), GameState::RunResults);
    const horace::RunStats& r = game.lastRun();
    EXPECT_EQ(r.number, 1);
    EXPECT_EQ(r.gatesPassed, 2);
    EXPECT_EQ(r.gatesMissed, 1);
    EXPECT_EQ(r.treesHit, 1);
    EXPECT_TRUE(r.finished);
    EXPECT_FALSE(r.skisBroken);
    EXPECT_EQ(r.points, 2 * Game::kGatePoints - Game::kGatePenalty + Game::kFinishBonus);
}

TEST(RunResultsTest, BrokenSkisAreReported) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    game.slope().startEmptyRun(10000);
    const float x = game.slope().skier().x;
    for (int i = 0; i < horace::SlopeLevel::kMaxDamage; ++i) {
        game.slope().addTree(x, 300 + i * 200.f);
    }
    rideWhileOnSlope(game);
    EXPECT_TRUE(game.lastRun().skisBroken);
    EXPECT_FALSE(game.lastRun().finished);
    EXPECT_EQ(game.lastRun().treesHit, horace::SlopeLevel::kMaxDamage);
}

TEST(RunResultsTest, NothingMovesOnResultsScreen) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    game.finishRun(false);
    const int money = game.money();
    game.update(5.f);
    game.moveHorace(Direction::Up);
    EXPECT_EQ(game.state(), GameState::RunResults);
    EXPECT_EQ(game.money(), money);
}

TEST(RunResultsTest, SecondRunHasNumberTwo) {
    Game game = gameWithEmptyRoad();
    goToSlope(game);
    game.finishRun(false);
    game.confirm();
    game.road().clearCars();
    walk(game, Direction::Down, kRowsDoorToBottom);
    ASSERT_EQ(game.state(), GameState::Slope);
    game.finishRun(false);
    EXPECT_EQ(game.lastRun().number, 2);
}

TEST(GameOverTest, EnterReturnsToFreshMenu) {
    Game game = gameWithEmptyRoad();
    loseAllMoney(game);
    ASSERT_EQ(game.state(), GameState::GameOver);
    game.confirm();
    EXPECT_EQ(game.state(), GameState::Menu);
    EXPECT_EQ(game.money(), Game::kStartMoney);
    EXPECT_EQ(game.score(), 0);
    EXPECT_EQ(game.runsCompleted(), 0);
}

TEST(GameOverTest, EnterDoesNothingDuringPlay) {
    Game game = gameWithEmptyRoad();
    game.confirm();
    EXPECT_EQ(game.state(), GameState::Road);
}
