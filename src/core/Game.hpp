#pragma once

#include "core/RoadLevel.hpp"

namespace horace {

// Состояния игры (конечный автомат):
//
//   Road --(дошёл до склона с лыжами)--> Slope --(заезд окончен)--> Road (у лавки)
//     |                                                                  |
//     +----------------(деньги кончились)----> GameOver <----------------+
enum class GameState { Road, Slope, GameOver };

// Правила игры: деньги, очки, переходы между состояниями.
// Ничего не знает про SFML, поэтому её легко тестировать.
class Game {
public:
    // Правила оригинала (Melbourne House, 1982)
    static constexpr int kStartMoney   = 40;
    static constexpr int kSkiPrice     = 10;
    static constexpr int kAmbulanceFee = 10;

    Game();

    void moveHorace(Direction dir);
    void update(float dt);

    // Заезд на склоне закончился. skisBroken — сломал ли Хорас лыжи.
    // Целые лыжи остаются, сломанные придётся брать в прокат снова.
    // TODO: пока вызывается по Enter; когда появится SlopeLevel — вызывать из update().
    void finishRun(bool skisBroken);

    GameState state() const { return state_; }
    int money() const { return money_; }
    int score() const { return score_; }

    RoadLevel& road() { return road_; }
    const RoadLevel& road() const { return road_; }

private:
    void payAmbulance();
    void rentSkis();
    void gameOver();

    GameState state_ = GameState::Road;
    int money_ = kStartMoney;
    int score_ = 0;
    RoadLevel road_;
};

} // namespace horace
