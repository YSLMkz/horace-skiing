#pragma once

#include "core/RoadLevel.hpp"
#include "core/SlopeLevel.hpp"

#include <vector>

namespace horace {

// Состояния игры (конечный автомат):
//
//   Road --(дошёл до склона с лыжами)--> Slope --(финиш / лыжи сломаны)--> Road (у лавки)
//     |                                                                       |
//     +-------------------(деньги кончились)----> GameOver <------------------+
enum class GameState { Road, Slope, GameOver };

// Что случилось в игре — для звуков (и любых других эффектов).
// Game только записывает события, а что с ними делать (какой звук играть),
// решает app. Так логика по-прежнему не знает про SFML.
enum class GameEvent {
    Step,        // Хорас сделал шаг по дороге
    Crash,       // сбила машина
    SkisRented,  // взял лыжи в прокат
    GatePassed,  // проехал ворота
    GateMissed,  // пропустил ворота
    TreeHit,     // врезался в дерево
    SkisBroken,  // сломал лыжи
    Finished,    // доехал до финиша
    GameOver,    // деньги кончились
    BonusMoney,  // набрал очередную 1000 очков — +$10
};

// Правила игры: деньги, очки, переходы между состояниями.
// Ничего не знает про SFML, поэтому её легко тестировать.
class Game {
public:
    // Правила оригинала (Melbourne House, 1982)
    static constexpr int kStartMoney   = 40;
    static constexpr int kSkiPrice     = 10;
    static constexpr int kAmbulanceFee = 10;
    static constexpr int kFinishBonus  = 100;  // за финиш (как в оригинале)
    static constexpr int kPointsForCash = 1000; // каждые 1000 очков…
    static constexpr int kCashBonus    = 10;   // …дают $10 (как в оригинале)
    // Очки за ворота — в оригинале точное число не указано, подобрано на глаз
    static constexpr int kGatePoints   = 10;   // за пройденные ворота
    static constexpr int kGatePenalty  = 10;   // штраф за пропущенные

    Game();

    // Управление на дороге (по шагу на нажатие)
    void moveHorace(Direction dir);
    // Управление на склоне (пока клавиша зажата)
    void setSteer(int dir);
    void setTuck(bool tuck);

    void update(float dt);

    // Пауза (в оригинале — клавиша S). Пока пауза, update() ничего не делает.
    void togglePause();
    bool isPaused() const { return paused_; }

    // Заезд окончен. skisBroken — сломал ли Хорас лыжи.
    // Целые лыжи остаются, сломанные придётся брать в прокат снова.
    void finishRun(bool skisBroken);

    GameState state() const { return state_; }
    int money() const { return money_; }
    int score() const { return score_; }
    int runsCompleted() const { return runsCompleted_; }
    // Сколько очков нужно набрать для следующих $10
    int nextCashAt() const { return nextCashAt_; }

    // Забрать накопившиеся события (список после этого очищается).
    // Вызывается раз в кадр.
    std::vector<GameEvent> takeEvents();

    RoadLevel& road() { return road_; }
    const RoadLevel& road() const { return road_; }
    SlopeLevel& slope() { return slope_; }
    const SlopeLevel& slope() const { return slope_; }

private:
    void updateRoad(float dt);
    void updateSlope(float dt);
    void startSlope();
    void payAmbulance();
    void rentSkis();
    void gameOver();
    void addScore(int points);  // очки не уходят ниже нуля; каждая 1000 — +$10
    void emit(GameEvent event) { events_.push_back(event); }

    GameState state_ = GameState::Road;
    int money_ = kStartMoney;
    int score_ = 0;
    int runsCompleted_ = 0;
    int nextCashAt_ = kPointsForCash;
    bool paused_ = false;
    unsigned nextCourseSeed_ = 1;
    RoadLevel road_;
    SlopeLevel slope_;
    std::vector<GameEvent> events_;
};

} // namespace horace
