#include "core/RoadLevel.hpp"

namespace horace {

RoadLevel::RoadLevel()
    : horace_(kCell, kCell),
      shop_{kWidth / 2.f - 1.5f * kCell, 0.f, 3.f * kCell, kCell} {
    // Полосы: соседние едут в разные стороны, скорость в пикселях в секунду.
    // TODO: по правилам оригинала машины иногда меняют полосу,
    //       а чем дольше Хорас стоит, тем больше машин.
    const float carW = kCell * 2.f;
    const float laneRows[] = {4, 6, 8, 10};
    const float laneSpeed[] = {120.f, -160.f, 200.f, -90.f};

    for (int lane = 0; lane < 4; ++lane) {
        for (int i = 0; i < 3; ++i) {
            const float x = i * (kWidth / 3.f) + lane * kCell;
            const int model = (lane + i) % kCarModels;  // чтобы в полосе были разные машины
            cars_.emplace_back(Rect{x, laneRows[lane] * kCell, carW, kCell}, laneSpeed[lane], model);
        }
    }
    startFromBottom();
}

void RoadLevel::startFromBottom() {
    ambulance_ = Ambulance{};
    hasSkis_ = false;
    horace_.setPosition(kStartX, kBottomY);
}

void RoadLevel::placeAtShop() {
    ambulance_ = Ambulance{};
    hasSkis_ = false;
    horace_.setPosition(kStartX, kShopDoorY);
}

void RoadLevel::giveSkis() {
    hasSkis_ = true;
    horace_.setPosition(kStartX, kShopDoorY);  // выходит из лавки на улицу
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

RoadEvent RoadLevel::update(float dt) {
    // Пока едет скорая, всё остальное на дороге стоит
    if (ambulance_.isActive()) {
        if (ambulance_.update(dt)) {
            respawnAfterHit();
            return RoadEvent::AmbulanceLeft;
        }
        return RoadEvent::None;
    }

    for (Car& car : cars_) {
        car.update(dt, kWidth);
    }

    for (const Car& car : cars_) {
        if (car.bounds().intersects(horace_.bounds())) {
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
}

} // namespace horace
