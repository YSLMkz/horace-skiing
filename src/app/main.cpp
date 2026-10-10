#include "app/Renderer.hpp"
#include "core/Game.hpp"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <iostream>

// HORACE_ASSETS_DIR задаётся в CMakeLists.txt — это путь к папке assets
#ifndef HORACE_ASSETS_DIR
#define HORACE_ASSETS_DIR "assets"
#endif

int main() {
    using horace::Direction;
    using horace::Game;
    using horace::GameState;
    using horace::RoadLevel;

    sf::RenderWindow window(
        sf::VideoMode(static_cast<unsigned>(RoadLevel::kWidth), static_cast<unsigned>(RoadLevel::kHeight)),
        "Horace Goes Skiing", sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);

    horace::Renderer renderer;
    if (!renderer.loadFont(std::string(HORACE_ASSETS_DIR) + "/fonts/DejaVuSansMono.ttf")) {
        std::cerr << "Font not found, text will not be shown\n";
    }

    Game game;
    sf::Clock clock;

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
                case sf::Keyboard::Enter:  game.finishRun(false);             break;  // временно
                case sf::Keyboard::B:      game.finishRun(true);              break;  // временно
                case sf::Keyboard::R:
                    if (game.state() == GameState::GameOver) {
                        game = Game();
                    }
                    break;
                case sf::Keyboard::Escape: window.close(); break;
                default: break;
                }
            }
        }

        // 2. Логика: dt — сколько секунд прошло с прошлого кадра,
        //    поэтому скорость машин не зависит от FPS.
        //    Ограничиваем сверху: пока окно перетаскивают, игра «замирает»,
        //    и без ограничения машины потом прыгнули бы на полэкрана.
        const float dt = std::min(clock.restart().asSeconds(), 0.1f);
        game.update(dt);

        // 3. Отрисовка
        renderer.draw(window, game);
        window.display();
    }
    return 0;
}
