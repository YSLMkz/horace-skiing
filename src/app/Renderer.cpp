#include "app/Renderer.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

namespace horace {

namespace {

const sf::Color kSnow(240, 240, 255);
const sf::Color kAsphalt(60, 60, 60);
const sf::Color kLaneMark(200, 200, 200);
const sf::Color kCar(220, 60, 60);
const sf::Color kShop(150, 90, 40);
const sf::Color kShopDoor(80, 45, 20);
const sf::Color kHorace(90, 200, 90);
const sf::Color kSkis(30, 60, 200);
const sf::Color kText(20, 20, 40);

void drawRect(sf::RenderTarget& target, const Rect& r, sf::Color color) {
    sf::RectangleShape shape({r.width, r.height});
    shape.setPosition(r.x, r.y);
    shape.setFillColor(color);
    target.draw(shape);
}

} // namespace

bool Renderer::loadFont(const std::string& path) {
    hasFont_ = font_.loadFromFile(path);
    return hasFont_;
}

void Renderer::draw(sf::RenderTarget& target, const Game& game) const {
    target.clear(kSnow);
    const float w = RoadLevel::kWidth;
    const float h = RoadLevel::kHeight;

    switch (game.state()) {
    case GameState::Road:
        drawRoad(target, game.road());
        break;
    case GameState::Slope:
        // Заглушка до следующего этапа
        drawText(target, "СКЛОН", w / 2, h / 2 - 60, 48, true);
        drawText(target, "Спуск ещё не готов — это следующий этап.", w / 2, h / 2, 20, true);
        drawText(target, "Enter — заезд окончен, лыжи целы", w / 2, h / 2 + 40, 20, true);
        drawText(target, "B — заезд окончен, лыжи сломаны", w / 2, h / 2 + 70, 20, true);
        break;
    case GameState::GameOver:
        drawRoad(target, game.road());
        drawRect(target, Rect{0, h / 2 - 70, w, 140}, sf::Color(255, 255, 255, 220));
        drawText(target, "ДЕНЬГИ КОНЧИЛИСЬ", w / 2, h / 2 - 50, 40, true);
        drawText(target, "R — начать заново, Esc — выход", w / 2, h / 2 + 10, 20, true);
        break;
    }
    drawHud(target, game);
}

void Renderer::drawRoad(sf::RenderTarget& target, const RoadLevel& road) const {
    const float cell = RoadLevel::kCell;

    // асфальт (строки 4..10) и пунктир между полосами
    drawRect(target, Rect{0, 4 * cell, RoadLevel::kWidth, 7 * cell}, kAsphalt);
    for (float row : {5.f, 7.f, 9.f}) {
        for (float x = 0; x < RoadLevel::kWidth; x += 2 * cell) {
            drawRect(target, Rect{x, row * cell + cell / 2 - 2, cell, 4}, kLaneMark);
        }
    }

    // лавка проката с дверью
    const Rect& shop = road.shop();
    drawRect(target, shop, kShop);
    drawRect(target, Rect{shop.x + shop.width / 2 - 10, shop.y + 10, 20, shop.height - 10}, kShopDoor);
    drawText(target, "ПРОКАТ $10", shop.x + shop.width + 10, shop.y + 8, 18);

    for (const Car& car : road.cars()) {
        drawRect(target, car.bounds(), kCar);
    }

    const Rect& hb = road.horace().bounds();
    drawRect(target, hb, kHorace);
    if (road.hasSkis()) {
        // лыжи на плече — две синие полоски
        drawRect(target, Rect{hb.x - 6, hb.y + 8, hb.width + 12, 5}, kSkis);
        drawRect(target, Rect{hb.x - 6, hb.y + 16, hb.width + 12, 5}, kSkis);
    }

    const char* hint = road.hasSkis() ? "Лыжи есть! Вниз, к склону" : "Дойди до лавки за лыжами";
    drawText(target, hint, 10, RoadLevel::kHeight - 30, 18);
}

void Renderer::drawHud(sf::RenderTarget& target, const Game& game) const {
    drawText(target, "$" + std::to_string(game.money()), 10, 8, 22);
    drawText(target, "Очки: " + std::to_string(game.score()), 10, 36, 18);
}

void Renderer::drawText(sf::RenderTarget& target, const std::string& utf8, float x, float y,
                        unsigned size, bool centered) const {
    if (!hasFont_) {
        return;
    }
    // Исходники в UTF-8, поэтому русский текст переводим через fromUtf8
    sf::Text text(sf::String::fromUtf8(utf8.begin(), utf8.end()), font_, size);
    text.setFillColor(kText);
    if (centered) {
        const sf::FloatRect b = text.getLocalBounds();
        text.setOrigin(b.left + b.width / 2.f, 0.f);
    }
    text.setPosition(x, y);
    target.draw(text);
}

} // namespace horace
