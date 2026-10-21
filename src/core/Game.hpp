#pragma once

#include "core/HighScores.hpp"
#include "core/RoadLevel.hpp"
#include "core/SlopeLevel.hpp"

#include <vector>

namespace horace {

// Состояния игры (конечный автомат):
//
//   Menu --Enter--> Road --(дошёл до склона)--> Slope --(финиш / лыжи сломаны)--> RunResults
//    ^               ^                                                             |
//    |               +--------------------------Enter (у лавки)--------------------+
//    |               |
//    |          (деньги кончились) --(попал в рекорды)--> EnterName --Enter--+
//    |               |                                                       |
//    +--Enter-- GameOver <-------------------------------------------------+
enum class GameState {
    Menu,        // стартовое меню
    Road,        // переход дороги
    Slope,       // спуск
    RunResults,  // итоги заезда
    EnterName,   // новый рекорд — вводим имя
    GameOver,    // деньги кончились
};

// Итоги одного заезда — для экрана после спуска
struct RunStats {
    int number = 0;        // номер заезда
    int gatesPassed = 0;
    int gatesMissed = 0;
    int treesHit = 0;
    bool finished = false;   // доехал до финиша
    bool skisBroken = false; // сломал лыжи
    int points = 0;          // сколько очков принёс заезд (с бонусом и штрафами)
    int cashBonus = 0;       // сколько $ заработано за 1000-очковые рубежи
};

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
    Confirm,     // нажали Enter в меню / на экране итогов
    HighScoreAdded,  // в таблицу рекордов добавлен результат — пора сохранить файл
    DemoModeOn,      // включили демо-режим
    DemoModeOff,     // выключили демо-режим
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

    // Демо-режим (для показа игры): машины не сбивают, деревья не ломают лыжи.
    // Включается и выключается, если нажать «секретную» клавишу kDemoPresses раз
    // подряд, не дольше kDemoPressGap секунд между нажатиями.
    static constexpr int   kDemoPresses  = 5;
    static constexpr float kDemoPressGap = 1.5f;

    Game();  // игра начинается со стартового меню

    // Нажатие «секретной» клавиши демо-режима (в app — клавиша «ё»)
    void pressDemoKey();
    bool isDemoMode() const { return demoMode_; }

    // Enter: в меню — начать игру, на итогах заезда — идти дальше,
    // после конца игры — вернуться в меню
    void confirm();

    // Управление на дороге (по шагу на нажатие)
    void moveHorace(Direction dir);
    // Управление на склоне (пока клавиша зажата)
    void setSteer(int dir);
    void setTuck(bool tuck);

    void update(float dt);

    // Пауза (в оригинале — клавиша S). Пока пауза, update() ничего не делает.
    void togglePause();
    bool isPaused() const { return paused_; }

    // Заезд окончен — показываем итоги. skisBroken — сломал ли Хорас лыжи.
    // После итогов Хорас окажется у лавки: целые лыжи остаются,
    // сломанные придётся брать в прокат снова.
    void finishRun(bool skisBroken);
    // Итоги последнего заезда
    const RunStats& lastRun() const { return lastRun_; }

    // Таблица рекордов. Загружает и сохраняет её app (файлы — не дело логики),
    // а Game решает, когда результат в неё попадает. Переживает начало новой игры.
    HighScores& highScores() { return highScores_; }
    const HighScores& highScores() const { return highScores_; }

    // Ввод имени для рекорда (в состоянии EnterName):
    // буквы (латиница и кириллица), цифры, пробел, '-', '_'; символ 8 — стереть.
    void typeNameChar(char32_t c);
    const std::string& nameInput() const { return nameInput_; }
    // Место в таблице (с 0) последнего добавленного рекорда; -1 — не попал
    int lastRank() const { return lastRank_; }

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
    void continueAfterResults();
    void payAmbulance();
    void rentSkis();
    void gameOver();
    void saveHighScore();
    void applyDemoMode(bool on);  // включить неуязвимость на дороге и склоне
    void addScore(int points);  // очки не уходят ниже нуля; каждая 1000 — +$10
    void emit(GameEvent event) { events_.push_back(event); }

    GameState state_ = GameState::Menu;
    int money_ = kStartMoney;
    int score_ = 0;
    int runsCompleted_ = 0;
    int nextCashAt_ = kPointsForCash;
    bool paused_ = false;
    unsigned nextCourseSeed_ = 1;
    RunStats run_;       // текущий заезд
    RunStats lastRun_;   // последний законченный
    int runStartScore_ = 0;
    int runStartMoney_ = 0;
    HighScores highScores_;
    std::string nameInput_;
    int lastRank_ = -1;
    bool demoMode_ = false;
    int demoPresses_ = 0;
    float sinceDemoPress_ = 0.f;
    RoadLevel road_;
    SlopeLevel slope_;
    std::vector<GameEvent> events_;
};

} // namespace horace
