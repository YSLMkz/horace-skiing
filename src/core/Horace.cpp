#include "core/Horace.hpp"

#include <algorithm>

namespace horace {

Horace::Horace(float size, float step)
    : bounds_{0.f, 0.f, size, size}, step_(step) {}

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

void Horace::setPosition(float x, float y) {
    bounds_.x = x;
    bounds_.y = y;
}

} // namespace horace
