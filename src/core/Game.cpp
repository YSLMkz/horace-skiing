#include "core/Game.hpp"

namespace horace {

namespace {
// Хорас стартует внизу по центру
constexpr float kStartX = Game::kFieldWidth / 2.f - Game::kCellSize / 2.f;
constexpr float kStartY = Game::kFieldHeight - Game::kCellSize;
} // namespace

Game::Game()
    : horace_(kStartX, kStartY, Game::kCellSize, Game::kCellSize) {
    // Пробная дорога: 4 полосы, соседние едут в разные стороны.
    // TODO: вынести дорогу в отдельный класс RoadLevel, добавить лавку с лыжами сверху.
    const float carW = kCellSize * 2.f;
    const float carH = kCellSize;
    const float laneY[] = {4 * kCellSize, 6 * kCellSize, 8 * kCellSize, 10 * kCellSize};
    const float laneSpeed[] = {120.f, -160.f, 200.f, -90.f};

    for (int lane = 0; lane < 4; ++lane) {
        for (int i = 0; i < 3; ++i) {
            const float x = i * (kFieldWidth / 3.f) + lane * kCellSize;
            cars_.emplace_back(Rect{x, laneY[lane], carW, carH}, laneSpeed[lane]);
        }
    }
}

void Game::moveHorace(Direction dir) {
    if (state_ != GameState::Road) {
        return;
    }
    horace_.move(dir, kFieldWidth, kFieldHeight);
}

void Game::update(float dt) {
    if (state_ != GameState::Road) {
        return;
    }
    for (Car& car : cars_) {
        car.update(dt, kFieldWidth);
    }
    for (const Car& car : cars_) {
        if (car.bounds().intersects(horace_.bounds())) {
            onHoraceHit();
            break;
        }
    }
}

void Game::onHoraceHit() {
    money_ -= kAmbulanceFee;
    horace_.resetToStart();
    if (money_ <= 0) {
        money_ = 0;
        state_ = GameState::GameOver;
    }
}

} // namespace horace
