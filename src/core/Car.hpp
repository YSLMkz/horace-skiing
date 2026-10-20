#pragma once

#include "core/Rect.hpp"

namespace horace {

// Машина на полосе. speed > 0 — едет вправо, speed < 0 — влево (пикселей в секунду).
// Уехав за край поля, появляется с другой стороны.
// model — номер модели (красная, жёлтая, синяя, фургон…): по нему выбирается текстура.
//
// Машина умеет перестраиваться в соседнюю полосу: changeLane() задаёт новую
// высоту и скорость. Машина плавно съезжает туда за несколько кадров
// со своей старой скоростью, а новую скорость берёт, когда доехала.
class Car {
public:
    static constexpr float kLaneChangeSpeed = 100.f;  // px/s по вертикали

    Car(Rect bounds, float speed, int model = 0);

    void update(float dt, float fieldWidth);

    // Начать перестроение в полосу на высоте laneY; доехав, ехать со скоростью newSpeed
    void changeLane(float laneY, float newSpeed);
    bool isChangingLane() const { return bounds_.y != targetY_; }
    // Полоса, в которой машина едет (или в которую перестраивается)
    float laneY() const { return targetY_; }

    const Rect& bounds() const { return bounds_; }
    float speed() const { return speed_; }
    int model() const { return model_; }

private:
    Rect bounds_;
    float speed_;
    int model_;
    float targetY_;
    float targetSpeed_;
};

} // namespace horace
