#pragma once

#include "core/Game.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <string>

namespace horace {

// Рисует текущее состояние игры. Только читает Game, ничего в нём не меняет.
// Пока всё — цветные прямоугольники; позже можно заменить на спрайты.
class Renderer {
public:
    // Возвращает false, если шрифт не найден (тогда текст просто не рисуется)
    bool loadFont(const std::string& path);

    void draw(sf::RenderTarget& target, const Game& game) const;

private:
    void drawRoad(sf::RenderTarget& target, const RoadLevel& road) const;
    void drawSlope(sf::RenderTarget& target, const SlopeLevel& slope) const;
    void drawHud(sf::RenderTarget& target, const Game& game) const;
    void drawText(sf::RenderTarget& target, const std::string& utf8, float x, float y,
                  unsigned size, bool centered = false) const;

    sf::Font font_;
    bool hasFont_ = false;
};

} // namespace horace
