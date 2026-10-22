#pragma once

#include "core/Game.hpp"

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Clock.hpp>

#include <map>
#include <string>
#include <vector>

namespace horace {

// Рисует текущее состояние игры. Только читает Game, ничего в нём не меняет.
// Машины и скорая — текстуры из assets/textures; если их нет, рисуются прямоугольники.
class Renderer {
public:
    // Возвращает false, если шрифт не найден (тогда текст просто не рисуется).
    // pixelFont = true — пиксельный шрифт: рисуется без сглаживания.
    bool loadFont(const std::string& path, bool pixelFont = false);
    // Возвращает false, если каких-то текстур нет (тогда вместо них — прямоугольники).
    // Спрайты героя, лыжника, ёлок, флажка и лавки необязательны: какого файла
    // нет, то и рисуется по-старому, прямоугольниками.
    bool loadTextures(const std::string& dir);

    void draw(sf::RenderTarget& target, const Game& game) const;

private:
    // withPlayer = false — только дорога и машины (фон меню)
    void drawRoad(sf::RenderTarget& target, const RoadLevel& road, bool withPlayer = true) const;
    void drawSlope(sf::RenderTarget& target, const SlopeLevel& slope) const;
    void drawCar(sf::RenderTarget& target, const Car& car) const;
    void drawHorace(sf::RenderTarget& target, const RoadLevel& road) const;
    void drawAmbulance(sf::RenderTarget& target, const Ambulance& ambulance) const;
    void drawHud(sf::RenderTarget& target, const Game& game) const;
    void drawMenu(sf::RenderTarget& target, const Game& game) const;
    void drawEnterName(sf::RenderTarget& target, const Game& game) const;
    // Таблица рекордов: строка highlight (место с 0) подсвечивается
    void drawHighScoreTable(sf::RenderTarget& target, const HighScores& table, float y,
                            int highlight) const;
    void drawRunResults(sf::RenderTarget& target, const Game& game) const;
    void drawGameOver(sf::RenderTarget& target, const Game& game) const;
    void drawPanel(sf::RenderTarget& target, const Rect& r) const;
    void drawBlinkingText(sf::RenderTarget& target, const std::string& utf8, float x, float y,
                          unsigned size) const;
    void drawText(sf::RenderTarget& target, const std::string& utf8, float x, float y,
                  unsigned size, bool centered = false,
                  sf::Color color = sf::Color(20, 20, 40)) const;

    sf::Font font_;
    bool hasFont_ = false;
    sf::Clock clock_;  // для мигающих надписей «Нажмите Enter»

    std::vector<sf::Texture> carTextures_;  // по одной на модель машины (Car::model())
    sf::Texture ambulanceFrames_[2];        // два кадра мигалки
    bool hasAmbulanceTextures_ = false;
    std::map<std::string, sf::Texture> sprites_;  // необязательные спрайты по имени файла
    const sf::Texture* sprite(const std::string& name) const;  // nullptr — файла нет
};

} // namespace horace
