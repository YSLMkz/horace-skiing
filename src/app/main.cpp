#include "app/Renderer.hpp"
#include "core/Game.hpp"

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <iostream>
#include <string>

// HORACE_ASSETS_DIR задаётся в CMakeLists.txt — это путь к папке assets
#ifndef HORACE_ASSETS_DIR
#define HORACE_ASSETS_DIR "assets"
#endif

namespace {

// Ищет файл assets/music/theme.* и включает его по кругу.
// Если файла нет — игра просто идёт без музыки.
bool startMusic(sf::Music& music) {
    const std::string dir = std::string(HORACE_ASSETS_DIR) + "/music/";
    for (const char* name : {"theme.ogg", "theme.mp3", "theme.wav", "theme.flac"}) {
        if (music.openFromFile(dir + name)) {
            music.setLoop(true);
            music.setVolume(50.f);  // 0..100
            music.play();
            return true;
        }
    }
    std::cerr << "Music not found: put theme.ogg (or .mp3/.wav/.flac) into " << dir << "\n";
    return false;
}

} // namespace

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
    if (!renderer.loadTextures(std::string(HORACE_ASSETS_DIR) + "/textures")) {
        std::cerr << "Some textures not found, drawing rectangles instead\n";
    }

    sf::Music music;
    const bool hasMusic = startMusic(music);

    Game game;
    sf::Clock clock;

    while (window.isOpen()) {
        // 1. Ввод: события (одно нажатие = одно действие)
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                case sf::Keyboard::Up:    game.moveHorace(Direction::Up);    break;
                case sf::Keyboard::Down:  game.moveHorace(Direction::Down);  break;
                case sf::Keyboard::Left:  game.moveHorace(Direction::Left);  break;
                case sf::Keyboard::Right: game.moveHorace(Direction::Right); break;
                case sf::Keyboard::M:  // музыка вкл/выкл
                    if (hasMusic) {
                        if (music.getStatus() == sf::Music::Playing) music.pause();
                        else music.play();
                    }
                    break;
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

        // Ввод: зажатые клавиши (на склоне едем, пока держим стрелку)
        const bool focused = window.hasFocus();
        const bool left  = focused && sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        const bool right = focused && sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
        const bool down  = focused && sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        game.setSteer((right ? 1 : 0) - (left ? 1 : 0));
        game.setTuck(down);

        // 2. Логика: dt — сколько секунд прошло с прошлого кадра,
        //    поэтому скорость не зависит от FPS.
        //    Ограничиваем сверху: пока окно перетаскивают, игра «замирает»,
        //    и без ограничения всё потом прыгнуло бы на полэкрана.
        const float dt = std::min(clock.restart().asSeconds(), 0.1f);
        game.update(dt);

        // 3. Отрисовка
        renderer.draw(window, game);
        window.display();
    }
    return 0;
}
