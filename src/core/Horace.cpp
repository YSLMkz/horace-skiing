#include "core/Horace.hpp"

#include <algorithm>

namespace horace {

Horace::Horace(float startX, float startY, float size, float step)
    : bounds_{startX, startY, size, size}, startX_(startX), startY_(startY), step_(step) {}

void Horace::move(Direction dir, float fieldWidth, float fieldHeight) {
    switch (dir) {
    case Direction::Up:    bounds_.y -= step_; break;
    case Direction::Down:  bounds_.y += step_; break;
    case Direction::Left:  bounds_.x -= step_; break;
    case Direction::Right: bounds_.x += step_; break;
    }
    // не даём выйти за край поля
    bounds_.x = std::clamp(bounds_.x, 0.f, fieldWidth - bounds_.width);
    bounds_.y = std::clamp(bounds_.y, 0.f, fieldHeight - bounds_.height);
}

void Horace::resetToStart() {
    bounds_.x = startX_;
    bounds_.y = startY_;
}

} // namespace horace
