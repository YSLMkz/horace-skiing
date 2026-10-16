#include "core/Ambulance.hpp"

namespace horace {

void Ambulance::call(const Rect& patient, float fieldWidth) {
    phase_ = AmbulancePhase::Arriving;
    fieldWidth_ = fieldWidth;
    time_ = 0.f;
    // Появляется из-за левого края на той же полосе, где лежит Хорас
    bounds_ = Rect{-kWidth, patient.y + patient.height - kHeight, kWidth, kHeight};
    // Останавливается так, чтобы передний бампер упёрся в Хораса
    targetX_ = patient.x - kWidth;
}

bool Ambulance::update(float dt) {
    if (phase_ == AmbulancePhase::Idle) {
        return false;
    }
    time_ += dt;

    switch (phase_) {
    case AmbulancePhase::Arriving:
        bounds_.x += kSpeed * dt;
        if (bounds_.x >= targetX_) {
            bounds_.x = targetX_;
            phase_ = AmbulancePhase::Loading;
            timer_ = kLoadingTime;
        }
        break;
    case AmbulancePhase::Loading:
        timer_ -= dt;
        if (timer_ <= 0.f) {
            phase_ = AmbulancePhase::Leaving;
        }
        break;
    case AmbulancePhase::Leaving:
        bounds_.x += kSpeed * dt;
        if (bounds_.x > fieldWidth_) {
            phase_ = AmbulancePhase::Idle;
            return true;
        }
        break;
    case AmbulancePhase::Idle:
        break;
    }
    return false;
}

} // namespace horace
