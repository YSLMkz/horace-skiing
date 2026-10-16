#pragma once

#include "core/Ambulance.hpp"
#include "core/Car.hpp"
#include "core/Horace.hpp"
#include "core/Rect.hpp"

#include <vector>

namespace horace {

// Что произошло на дороге за один кадр. Решения (списать деньги, сменить
// состояние игры) принимает Game — дорога только сообщает о событии.
enum class RoadEvent {
    None,
    HitByCar,       // сбила машина — едет скорая, движение замерло
    AmbulanceLeft,  // скорая уехала, Хораса вернули на исходную сторону — пора платить
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
    static constexpr int   kCarModels = 4;  // сколько разных моделей машин (и текстур)

    RoadLevel();

    void startFromBottom();  // новая игра: внизу, без лыж
    void placeAtShop();      // после заезда: у двери лавки, без лыж
    void giveSkis();         // лыжи получены — теперь надо идти вниз на склон

    // Пока едет скорая, Хорас не слушается
    void moveHorace(Direction dir);
    RoadEvent update(float dt);

    bool hasSkis() const { return hasSkis_; }
    const Horace& horace() const { return horace_; }
    const std::vector<Car>& cars() const { return cars_; }
    const Rect& shop() const { return shop_; }
    const Ambulance& ambulance() const { return ambulance_; }
    // Хорас лежит на дороге и ждёт скорую
    bool isHoraceKnockedDown() const;
    // Хорас уже в скорой (его не надо рисовать)
    bool isHoraceInAmbulance() const;

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
    Ambulance ambulance_;
    bool hasSkis_ = false;
};

} // namespace horace
