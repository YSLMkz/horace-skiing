#pragma once

#include "core/Rect.hpp"

namespace horace {

enum class Direction { Up, Down, Left, Right };

// Главный герой. На дороге ходит шагами (как в оригинале),
// не может выйти за пределы поля.
class Horace {
public:
    Horace(float size, float step);

    void move(Direction dir, float fieldWidth, float fieldHeight);
    void setPosition(float x, float y);

    const Rect& bounds() const { return bounds_; }

private:
    Rect bounds_;
    float step_;
};

} // namespace horace
