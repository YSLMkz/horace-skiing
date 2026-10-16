#pragma once

#include "core/Rect.hpp"

namespace horace {

// Машина на полосе. speed > 0 — едет вправо, speed < 0 — влево (пикселей в секунду).
// Уехав за край поля, появляется с другой стороны.
// model — номер модели (красная, жёлтая, синяя, фургон…): по нему выбирается текстура.
class Car {
public:
    Car(Rect bounds, float speed, int model = 0);

    void update(float dt, float fieldWidth);

    const Rect& bounds() const { return bounds_; }
    float speed() const { return speed_; }
    int model() const { return model_; }

private:
    Rect bounds_;
    float speed_;
    int model_;
};

} // namespace horace
