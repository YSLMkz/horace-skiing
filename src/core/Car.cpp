#include "core/Car.hpp"

namespace horace {

Car::Car(Rect bounds, float speed) : bounds_(bounds), speed_(speed) {}

void Car::update(float dt, float fieldWidth) {
    bounds_.x += speed_ * dt;

    // полностью уехала вправо — появляется слева, и наоборот
    if (speed_ > 0.f && bounds_.x > fieldWidth) {
        bounds_.x = -bounds_.width;
    } else if (speed_ < 0.f && bounds_.x + bounds_.width < 0.f) {
        bounds_.x = fieldWidth;
    }
}

} // namespace horace
