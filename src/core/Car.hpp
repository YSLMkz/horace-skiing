#pragma once

#include "core/Rect.hpp"

namespace horace {

// Машина на полосе. speed > 0 — едет вправо, speed < 0 — влево (пикселей в секунду).
// Уехав за край поля, появляется с другой стороны.
class Car {
public:
    Car(Rect bounds, float speed);

    void update(float dt, float fieldWidth);

    const Rect& bounds() const { return bounds_; }
    float speed() const { return speed_; }

private:
    Rect bounds_;
    float speed_;
};

} // namespace horace
