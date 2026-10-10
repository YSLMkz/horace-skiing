#pragma once

#include "core/RoadLevel.hpp"
#include "core/SlopeLevel.hpp"

namespace horace {

// Состояния игры (конечный автомат):
//
//   Road --(дошёл до склона с лыжами)--> Slope --(финиш / лыжи сломаны)--> Road (у лавки)
//     |                                                                       |
//     +-------------------(деньги кончились)----> GameOver <------------------+
enum class GameState { Road, Slope, GameOver };

// Правила игры: деньги, очки, переходы между состояниями.
// Ничего не знает про SFML, поэтому её легко тестировать.
class Game {
public:
    // Правила оригинала (Melbourne House, 1982)
    static constexpr int kStartMoney   = 40;
    static constexpr int kSkiPrice     = 10;
    static constexpr int kAmbulanceFee = 10;
    // Очки — TODO: сверить с оригиналом
    static constexpr int kGatePoints   = 10;   // за каждые ворота
    static constexpr int kFinishBonus  = 100;  // за финиш

    Game();

    // Управление на дороге (по шагу на нажатие)
    void moveHorace(Direction dir);
    // Управление на склоне (пока клавиша зажата)
    void setSteer(int dir);
    void setTuck(bool tuck);

    void update(float dt);

    // Заезд окончен. skisBroken — сломал ли Хорас лыжи.
    // Целые лыжи остаются, сломанные придётся брать в прокат снова.
    void finishRun(bool skisBroken);

    GameState state() const { return state_; }
    int money() const { return money_; }
    int score() const { return score_; }
    int runsCompleted() const { return runsCompleted_; }

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

    GameState state_ = GameState::Road;
    int money_ = kStartMoney;
    int score_ = 0;
    int runsCompleted_ = 0;
    unsigned nextCourseSeed_ = 1;
    RoadLevel road_;
    SlopeLevel slope_;
};

} // namespace horace
