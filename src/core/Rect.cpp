#include "core/Rect.hpp"

namespace horace {

bool Rect::intersects(const Rect& other) const {
    return x < other.x + other.width  && other.x < x + width &&
           y < other.y + other.height && other.y < y + height;
}

} // namespace horace
