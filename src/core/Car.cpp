#include "core/Car.hpp"

#include <algorithm>

namespace horace {

Car::Car(Rect bounds, float speed, int model)
    : bounds_(bounds), speed_(speed), model_(model), targetY_(bounds.y), targetSpeed_(speed) {}

void Car::changeLane(float laneY, float newSpeed) {
    targetY_ = laneY;
    targetSpeed_ = newSpeed;
}

void Car::update(float dt, float fieldWidth) {
    bounds_.x += speed_ * dt;

    // полностью уехала вправо — появляется слева, и наоборот
    if (speed_ > 0.f && bounds_.x > fieldWidth) {
        bounds_.x = -bounds_.width;
    } else if (speed_ < 0.f && bounds_.x + bounds_.width < 0.f) {
        bounds_.x = fieldWidth;
    }

    // перестроение: плавно съезжаем к нужной полосе, не проскакивая её
    if (bounds_.y < targetY_) {
        bounds_.y = std::min(targetY_, bounds_.y + kLaneChangeSpeed * dt);
    } else if (bounds_.y > targetY_) {
        bounds_.y = std::max(targetY_, bounds_.y - kLaneChangeSpeed * dt);
    }
    if (bounds_.y == targetY_) {
        speed_ = targetSpeed_;  // доехали до новой полосы — едем в её темпе
    }
}

} // namespace horace
