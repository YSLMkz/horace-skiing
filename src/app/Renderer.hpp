#pragma once

#include "core/Game.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <string>
#include <vector>

namespace horace {

// Рисует текущее состояние игры. Только читает Game, ничего в нём не меняет.
// Машины и скорая — текстуры из assets/textures; если их нет, рисуются прямоугольники.
class Renderer {
public:
    // Возвращает false, если шрифт не найден (тогда текст просто не рисуется)
    bool loadFont(const std::string& path);
    // Возвращает false, если каких-то текстур нет (тогда вместо них — прямоугольники)
    bool loadTextures(const std::string& dir);

    void draw(sf::RenderTarget& target, const Game& game) const;

private:
    void drawRoad(sf::RenderTarget& target, const RoadLevel& road) const;
    void drawSlope(sf::RenderTarget& target, const SlopeLevel& slope) const;
    void drawCar(sf::RenderTarget& target, const Car& car) const;
    void drawHorace(sf::RenderTarget& target, const RoadLevel& road) const;
    void drawAmbulance(sf::RenderTarget& target, const Ambulance& ambulance) const;
    void drawHud(sf::RenderTarget& target, const Game& game) const;
    void drawText(sf::RenderTarget& target, const std::string& utf8, float x, float y,
                  unsigned size, bool centered = false) const;

    sf::Font font_;
    bool hasFont_ = false;

    std::vector<sf::Texture> carTextures_;  // по одной на модель машины (Car::model())
    sf::Texture ambulanceFrames_[2];        // два кадра мигалки
    bool hasAmbulanceTextures_ = false;
};

} // namespace horace
