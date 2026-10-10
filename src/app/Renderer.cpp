#include "app/Renderer.hpp"

#include <SFML/Graphics/RectangleShape.hpp>

namespace horace {

namespace {
void drawRect(sf::RenderTarget& target, const Rect& r, sf::Color color) {
    sf::RectangleShape shape({r.width, r.height});
    shape.setPosition(r.x, r.y);
    shape.setFillColor(color);
    target.draw(shape);
}
} // namespace

void Renderer::draw(sf::RenderTarget& target, const Game& game) const {
    // асфальт под полосами
    drawRect(target, Rect{0.f, 4 * Game::kCellSize, Game::kFieldWidth, 7 * Game::kCellSize},
             sf::Color(60, 60, 60));

    for (const Car& car : game.cars()) {
        drawRect(target, car.bounds(), sf::Color(220, 60, 60));
    }

    const sf::Color horaceColor =
        game.state() == GameState::GameOver ? sf::Color(120, 120, 120) : sf::Color(90, 200, 90);
    drawRect(target, game.horace().bounds(), horaceColor);
}

} // namespace horace
