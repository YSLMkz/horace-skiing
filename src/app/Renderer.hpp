#pragma once

#include "core/Game.hpp"

#include <SFML/Graphics/RenderTarget.hpp>

namespace horace {

// Рисует текущее состояние игры. Только читает Game, ничего в нём не меняет.
// Пока всё — цветные прямоугольники; позже можно заменить на спрайты.
class Renderer {
public:
    void draw(sf::RenderTarget& target, const Game& game) const;
};

} // namespace horace
