#include "core/RoadLevel.hpp"

#include <algorithm>
#include <cmath>

namespace horace {

namespace {

struct Lane {
    float row;    // номер строки сетки
    float speed;  // px/s, знак — направление
};

// Две полосы вправо (сверху) и две влево (снизу), как на настоящей дороге
const Lane kLanes[RoadLevel::kLaneCount] = {
    {4, 110.f}, {6, 170.f},     // вправо
    {8, -150.f}, {10, -95.f},   // влево
};

// Перестроение — раз в 1.5…4 секунды
constexpr float kLaneChangeMin = 1.5f;
constexpr float kLaneChangeMax = 4.f;

bool overlapsVertically(const Rect& r, float laneY) {
    return r.y < laneY + RoadLevel::kCell && laneY < r.y + r.height;
}

// Расстояние между машинами по горизонтали «по кругу»: дорога закольцована —
// машина, уехавшая за правый край, появляется слева (период = ширина поля + машина).
// Поэтому машина у самого правого края на самом деле стоит прямо перед въездом слева.
float ringDistance(float x1, float x2) {
    const float period = RoadLevel::kWidth + RoadLevel::kCarWidth;
    float d = std::fmod(std::abs(x1 - x2), period);
    return std::min(d, period - d);
}

} // namespace

float RoadLevel::laneY(int lane) { return kLanes[lane].row * kCell; }
float RoadLevel::laneSpeed(int lane) { return kLanes[lane].speed; }

int RoadLevel::laneIndex(float y) {
    for (int lane = 0; lane < kLaneCount; ++lane) {
        if (laneY(lane) == y) return lane;
    }
    return -1;
}

RoadLevel::RoadLevel()
    : horace_(kCell, kCell),
      shop_{kWidth / 2.f - kShopWidth / 2.f, 0.f, kShopWidth, kShopHeight} {
    startFromBottom();
}

void RoadLevel::resetTraffic() {
    cars_.clear();
    for (int lane = 0; lane < kLaneCount; ++lane) {
        for (int i = 0; i < kBaseCarsPerLane; ++i) {
            const float x = i * (kWidth / kBaseCarsPerLane) + lane * kCell;
            const int model = (lane + i) % kCarModels;  // чтобы в полосе были разные машины
            cars_.emplace_back(Rect{x, laneY(lane), kCarWidth, kCell}, laneSpeed(lane), model);
        }
    }
}

void RoadLevel::startNewCrossing() {
    crossingTime_ = 0.f;
    spawnTimer_ = kSpawnInterval;
}

void RoadLevel::startFromBottom() {
    ambulance_ = Ambulance{};
    hasSkis_ = false;
    horace_.setPosition(kStartX, kBottomY);
    resetTraffic();
    startNewCrossing();
}

void RoadLevel::placeAtShop() {
    ambulance_ = Ambulance{};
    hasSkis_ = false;
    horace_.setPosition(kStartX, kShopDoorY);
    resetTraffic();
    startNewCrossing();
}

void RoadLevel::giveSkis() {
    hasSkis_ = true;
    horace_.setPosition(kStartX, kShopDoorY);  // выходит из лавки на улицу
    startNewCrossing();                        // обратный путь — новый переход
}

bool RoadLevel::moveHorace(Direction dir) {
    if (ambulance_.isActive()) {
        return false;
    }
    const Rect before = horace_.bounds();
    horace_.move(dir, kWidth, kHeight);
    return horace_.bounds().x != before.x || horace_.bounds().y != before.y;
}

bool RoadLevel::isHoraceKnockedDown() const {
    return ambulance_.phase() == AmbulancePhase::Arriving ||
           ambulance_.phase() == AmbulancePhase::Loading;
}

bool RoadLevel::isHoraceInAmbulance() const {
    return ambulance_.phase() == AmbulancePhase::Leaving;
}

int RoadLevel::carsInLane(int lane) const {
    int count = 0;
    for (const Car& car : cars_) {
        if (car.laneY() == laneY(lane)) ++count;
    }
    return count;
}

bool RoadLevel::laneIsFreeNear(int lane, float x, const Car* except, float extraGap) const {
    const float y = laneY(lane);
    for (const Car& car : cars_) {
        if (&car == except) continue;
        // мешает машина, которая едет в этой полосе, перестраивается в неё
        // или ещё не до конца из неё выехала
        const bool inLane = car.laneY() == y || overlapsVertically(car.bounds(), y);
        if (inLane && ringDistance(car.bounds().x, x) < kCarWidth + kCarGap + extraGap) {
            return false;
        }
    }
    return true;
}

bool RoadLevel::trySpawnCar() {
    // Полосы по очереди: каждый раз пробуем следующую
    for (int attempt = 0; attempt < kLaneCount; ++attempt) {
        const int lane = (spawned_ + attempt) % kLaneCount;
        if (carsInLane(lane) >= kMaxCarsPerLane) continue;

        // машина въезжает из-за края, откуда едет её полоса
        const float x = laneSpeed(lane) > 0.f ? -kCarWidth : kWidth;
        if (!laneIsFreeNear(lane, x, nullptr)) continue;

        cars_.emplace_back(Rect{x, laneY(lane), kCarWidth, kCell}, laneSpeed(lane),
                           spawned_ % kCarModels);
        ++spawned_;  // следующую машину — в следующую полосу и другой модели
        return true;
    }
    return false;
}

bool RoadLevel::tryChangeLane(size_t index) {
    if (index >= cars_.size()) return false;
    Car& car = cars_[index];
    if (car.isChangingLane()) return false;

    const int lane = laneIndex(car.laneY());
    if (lane < 0) return false;
    const int target = lane ^ 1;  // 0 <-> 1, 2 <-> 3: соседняя полоса того же направления

    if (carsInLane(target) >= kMaxCarsPerLane) return false;  // полоса и так забита

    // Пока машина перестраивается, она едет со старой скоростью, а соседи
    // в новой полосе — со своей. За это время расстояние может сократиться —
    // закладываем запас.
    const float changeTime = kCell / Car::kLaneChangeSpeed;
    const float closing = std::abs(car.speed() - laneSpeed(target)) * changeTime;
    if (!laneIsFreeNear(target, car.bounds().x, &car, closing)) return false;
    car.changeLane(laneY(target), laneSpeed(target));
    return true;
}

void RoadLevel::updateTraffic(float dt) {
    crossingTime_ += dt;

    // Чем дольше идём — тем больше машин
    spawnTimer_ -= dt;
    if (spawnTimer_ <= 0.f) {
        trySpawnCar();
        spawnTimer_ += kSpawnInterval;
    }

    // Иногда случайная машина перестраивается
    laneChangeTimer_ -= dt;
    if (laneChangeTimer_ <= 0.f && !cars_.empty()) {
        std::uniform_int_distribution<size_t> pick(0, cars_.size() - 1);
        const size_t first = pick(rng_);
        for (size_t k = 0; k < cars_.size(); ++k) {
            if (tryChangeLane((first + k) % cars_.size())) break;
        }
        std::uniform_real_distribution<float> next(kLaneChangeMin, kLaneChangeMax);
        laneChangeTimer_ = next(rng_);
    }

    for (Car& car : cars_) {
        car.update(dt, kWidth);
    }
}

RoadEvent RoadLevel::update(float dt) {
    // Пока едет скорая, всё остальное на дороге стоит
    if (ambulance_.isActive()) {
        if (ambulance_.update(dt)) {
            respawnAfterHit();
            return RoadEvent::AmbulanceLeft;
        }
        return RoadEvent::None;
    }

    updateTraffic(dt);

    for (const Car& car : cars_) {
        if (!invincible_ && car.bounds().intersects(horace_.bounds())) {
            ambulance_.call(horace_.bounds(), kWidth);
            return RoadEvent::HitByCar;
        }
    }

    if (!hasSkis_ && horace_.bounds().intersects(shop_)) {
        return RoadEvent::ReachedShop;
    }
    if (hasSkis_ && horace_.bounds().y >= kBottomY) {
        return RoadEvent::ReachedSlope;
    }
    return RoadEvent::None;
}

void RoadLevel::respawnAfterHit() {
    // Скорая привозит Хораса на ту сторону, откуда он шёл:
    // без лыж — вниз на старт, с лыжами — к лавке (лыжи остаются при нём).
    if (hasSkis_) {
        horace_.setPosition(kStartX, kShopDoorY);
    } else {
        horace_.setPosition(kStartX, kBottomY);
    }
    startNewCrossing();
}

} // namespace horace
