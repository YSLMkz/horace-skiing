#include "core/Game.hpp"

namespace horace {

Game::Game() = default;

void Game::moveHorace(Direction dir) {
    if (state_ == GameState::Road) {
        road_.moveHorace(dir);
    }
}

void Game::update(float dt) {
    if (state_ != GameState::Road) {
        return;  // TODO: здесь будет slope_.update(dt) для состояния Slope
    }

    switch (road_.update(dt)) {
    case RoadEvent::HitByCar:
        payAmbulance();
        break;
    case RoadEvent::ReachedShop:
        rentSkis();
        break;
    case RoadEvent::ReachedSlope:
        state_ = GameState::Slope;
        break;
    case RoadEvent::None:
        break;
    }
}

void Game::finishRun(bool skisBroken) {
    if (state_ != GameState::Slope) {
        return;
    }
    state_ = GameState::Road;
    road_.placeAtShop();
    if (!skisBroken) {
        road_.giveSkis();  // лыжи целы — сразу идём обратно к склону
    }
    // сломаны — Хорас стоит у двери лавки и должен зайти за новыми
}

void Game::payAmbulance() {
    money_ -= kAmbulanceFee;
    if (money_ <= 0) {
        gameOver();
    }
}

void Game::rentSkis() {
    if (money_ < kSkiPrice) {
        gameOver();
        return;
    }
    money_ -= kSkiPrice;
    road_.giveSkis();
}

void Game::gameOver() {
    money_ = 0;
    state_ = GameState::GameOver;
}

} // namespace horace
