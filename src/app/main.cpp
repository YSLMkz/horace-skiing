#include "app/Renderer.hpp"
#include "core/Game.hpp"

#include <SFML/Graphics.hpp>

#include <string>

namespace {

// Деньги и состояние пока выводим в заголовок окна — так не нужен файл шрифта.
// TODO: заменить на sf::Text со своим шрифтом.
std::string makeTitle(const horace::Game& game) {
    std::string title = "Horace Goes Skiing | $" + std::to_string(game.money());
    if (game.state() == horace::GameState::GameOver) {
        title += " | GAME OVER (Esc - exit)";
    }
    return title;
}

} // namespace

int main() {
    using horace::Direction;
    using horace::Game;

    sf::RenderWindow window(
        sf::VideoMode(static_cast<unsigned>(Game::kFieldWidth), static_cast<unsigned>(Game::kFieldHeight)),
        "Horace Goes Skiing", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    Game game;
    horace::Renderer renderer;
    sf::Clock clock;
    std::string lastTitle;

    while (window.isOpen()) {
        // 1. Ввод
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                case sf::Keyboard::Up:     game.moveHorace(Direction::Up);    break;
                case sf::Keyboard::Down:   game.moveHorace(Direction::Down);  break;
                case sf::Keyboard::Left:   game.moveHorace(Direction::Left);  break;
                case sf::Keyboard::Right:  game.moveHorace(Direction::Right); break;
                case sf::Keyboard::Escape: window.close();                    break;
                default: break;
                }
            }
        }

        // 2. Логика: dt — сколько секунд прошло с прошлого кадра,
        //    поэтому скорость машин не зависит от FPS
        const float dt = clock.restart().asSeconds();
        game.update(dt);

        const std::string title = makeTitle(game);
        if (title != lastTitle) {
            window.setTitle(title);
            lastTitle = title;
        }

        // 3. Отрисовка
        window.clear(sf::Color(240, 240, 255));  // снег
        renderer.draw(window, game);
        window.display();
    }
    return 0;
}
