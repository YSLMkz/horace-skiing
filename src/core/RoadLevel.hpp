#pragma once

#include "core/Car.hpp"
#include "core/Horace.hpp"
#include "core/Rect.hpp"

#include <vector>

namespace horace {

// Что произошло на дороге за один кадр. Решения (списать деньги, сменить
// состояние игры) принимает Game — дорога только сообщает о событии.
enum class RoadEvent {
    None,
    HitByCar,      // сбила машина (Хораса уже вернули на исходную сторону)
    ReachedShop,   // зашёл в лавку без лыж — пора покупать
    ReachedSlope,  // перешёл дорогу обратно с лыжами — пора на склон
};

// Уровень «Дорога»: Хорас, машины и лавка проката лыж.
//
// Поле — сетка 20 x 15 клеток по 40 px:
//   строка 0      — лавка (по центру)
//   строки 4..10  — дорога с полосами
//   строка 14     — старт, отсюда же вход на склон
class RoadLevel {
public:
    static constexpr float kWidth  = 800.f;
    static constexpr float kHeight = 600.f;
    static constexpr float kCell   = 40.f;

    RoadLevel();

    void startFromBottom();  // новая игра: внизу, без лыж
    void placeAtShop();      // после заезда: у двери лавки, без лыж
    void giveSkis();         // лыжи получены — теперь надо идти вниз на склон

    void moveHorace(Direction dir);
    RoadEvent update(float dt);

    bool hasSkis() const { return hasSkis_; }
    const Horace& horace() const { return horace_; }
    const std::vector<Car>& cars() const { return cars_; }
    const Rect& shop() const { return shop_; }

    // Для тестов: убрать машины / поставить свою
    void clearCars() { cars_.clear(); }
    void addCar(const Car& car) { cars_.push_back(car); }

    // Координаты, которые нужны и логике, и тестам
    static constexpr float kStartX     = kWidth / 2.f - kCell / 2.f;  // по центру
    static constexpr float kBottomY    = kHeight - kCell;             // нижняя строка
    static constexpr float kShopDoorY  = kCell;                       // строка под лавкой

private:
    void respawnAfterHit();

    Horace horace_;
    std::vector<Car> cars_;
    Rect shop_;
    bool hasSkis_ = false;
};

} // namespace horace
