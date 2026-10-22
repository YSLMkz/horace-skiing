#pragma once

#include "core/Ambulance.hpp"
#include "core/Car.hpp"
#include "core/Horace.hpp"
#include "core/Rect.hpp"

#include <random>
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
//   строки 0..1   — лавка (по центру), строка 2 — у её двери
//   строки 4..10  — дорога: полосы 4 и 6 едут вправо, 8 и 10 — влево
//   строка 14     — старт, отсюда же вход на склон
//
// Как в оригинале: машины иногда перестраиваются в соседнюю полосу
// (того же направления), а чем дольше Хорас переходит дорогу,
// тем больше становится машин.
class RoadLevel {
public:
    static constexpr float kWidth  = 800.f;
    static constexpr float kHeight = 600.f;
    static constexpr float kCell   = 40.f;
    static constexpr int   kCarModels = 4;  // сколько разных моделей машин (и текстур)

    static constexpr int   kLaneCount       = 4;
    static constexpr int   kBaseCarsPerLane = 3;     // столько машин в полосе в начале перехода
    static constexpr int   kMaxCarsPerLane  = 5;     // больше не добавляем
    static constexpr float kSpawnInterval   = 6.f;   // раз в столько секунд — новая машина
    static constexpr float kCarWidth        = 2 * kCell;
    static constexpr float kCarGap          = kCell; // минимальный зазор между машинами в полосе

    static float laneY(int lane);      // высота полосы
    static float laneSpeed(int lane);  // скорость машин в полосе (знак — направление)
    static int laneIndex(float y);     // номер полосы по высоте или -1

    RoadLevel();

    void startFromBottom();  // новая игра: внизу, без лыж
    void placeAtShop();      // после заезда: у двери лавки, без лыж
    void giveSkis();         // лыжи получены — теперь надо идти вниз на склон

    // Возвращает true, если Хорас действительно сдвинулся
    // (не сдвинется, если упёрся в край или пока едет скорая)
    bool moveHorace(Direction dir);
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

    // Демо-режим: машины проезжают сквозь Хораса
    void setInvincible(bool on) { invincible_ = on; }
    bool isInvincible() const { return invincible_; }

    // Сколько секунд Хорас уже переходит дорогу (от этого растёт трафик)
    float crossingTime() const { return crossingTime_; }

    // Добавить машину на въезде в какую-нибудь полосу, если там есть место
    bool trySpawnCar();
    // Перестроить машину номер index в соседнюю полосу того же направления,
    // если там свободно. Возвращает true, если перестроение началось.
    bool tryChangeLane(size_t index);

    // Для тестов: убрать машины / поставить свою
    void clearCars() { cars_.clear(); }
    void addCar(const Car& car) { cars_.push_back(car); }

    // Координаты, которые нужны и логике, и тестам
    static constexpr float kStartX     = kWidth / 2.f - kCell / 2.f;  // по центру
    static constexpr float kBottomY    = kHeight - kCell;             // нижняя строка
    static constexpr float kShopHeight = 2 * kCell;                   // лавка — две клетки в высоту
    static constexpr float kShopWidth  = 5 * kCell;                   // и пять в ширину
    static constexpr float kShopDoorY  = kShopHeight;                 // строка под лавкой

private:
    void respawnAfterHit();
    void resetTraffic();        // вернуть начальные машины
    void startNewCrossing();    // обнулить «сколько уже идём»
    void updateTraffic(float dt);
    int carsInLane(int lane) const;
    bool laneIsFreeNear(int lane, float x, const Car* except, float extraGap = 0.f) const;

    Horace horace_;
    std::vector<Car> cars_;
    Rect shop_;
    Ambulance ambulance_;
    bool hasSkis_ = false;
    bool invincible_ = false;

    float crossingTime_ = 0.f;
    float spawnTimer_ = kSpawnInterval;
    float laneChangeTimer_ = 2.f;
    int spawned_ = 0;
    std::mt19937 rng_{2024};  // фиксированный seed — поведение машин повторяемо
};

} // namespace horace
