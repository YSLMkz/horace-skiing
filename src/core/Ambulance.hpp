#pragma once

#include "core/Rect.hpp"

namespace horace {

// Этапы поездки скорой
enum class AmbulancePhase {
    Idle,      // скорой нет на экране
    Arriving,  // едет слева к сбитому Хорасу
    Loading,   // стоит и грузит Хораса
    Leaving,   // уезжает вправо с Хорасом внутри
};

// Скорая помощь. Вызывается, когда Хораса сбила машина:
// приезжает по его полосе слева, стоит немного и уезжает за правый край.
// Пока скорая на экране, Хорас не управляется, а движение на дороге замирает.
class Ambulance {
public:
    static constexpr float kWidth       = 80.f;
    static constexpr float kHeight      = 40.f;
    static constexpr float kSpeed       = 450.f;  // px/s
    static constexpr float kLoadingTime = 0.7f;   // сколько секунд грузят Хораса

    // Вызвать скорую к пострадавшему (patient — где лежит Хорас)
    void call(const Rect& patient, float fieldWidth);

    // Возвращает true в тот кадр, когда скорая уехала за край экрана
    bool update(float dt);

    AmbulancePhase phase() const { return phase_; }
    bool isActive() const { return phase_ != AmbulancePhase::Idle; }
    const Rect& bounds() const { return bounds_; }
    // Сколько секунд скорая уже на экране — для мигалки
    float time() const { return time_; }

private:
    AmbulancePhase phase_ = AmbulancePhase::Idle;
    Rect bounds_;
    float targetX_ = 0.f;
    float fieldWidth_ = 0.f;
    float timer_ = 0.f;
    float time_ = 0.f;
};

} // namespace horace
