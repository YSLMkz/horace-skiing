#pragma once

#include "core/Car.hpp"
#include "core/Horace.hpp"

#include <vector>

namespace horace {

// Состояния игры (конечный автомат).
// TODO: добавить Menu, Slope (спуск с горы), Results и переходы между ними.
enum class GameState { Road, GameOver };

// Вся логика игры. Ничего не знает про SFML, поэтому её легко тестировать.
class Game {
public:
    static constexpr float kFieldWidth   = 800.f;
    static constexpr float kFieldHeight  = 600.f;
    static constexpr float kCellSize     = 40.f;   // размер клетки = шаг Хораса
    static constexpr int   kStartMoney   = 40;     // TODO: сверить с оригиналом
    static constexpr int   kAmbulanceFee = 10;     // плата за скорую при столкновении

    Game();

    void moveHorace(Direction dir);
    void update(float dt);

    // Для тестов: добавить свою машину (по умолчанию дорога уже заполнена)
    void clearCars() { cars_.clear(); }
    void addCar(const Car& car) { cars_.push_back(car); }

    GameState state() const { return state_; }
    int money() const { return money_; }
    const Horace& horace() const { return horace_; }
    const std::vector<Car>& cars() const { return cars_; }

private:
    void onHoraceHit();

    GameState state_ = GameState::Road;
    int money_ = kStartMoney;
    Horace horace_;
    std::vector<Car> cars_;
};

} // namespace horace
