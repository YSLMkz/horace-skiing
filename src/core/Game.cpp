#include "core/Game.hpp"

namespace horace {

Game::Game() = default;

void Game::moveHorace(Direction dir) {
    if (state_ == GameState::Road) {
        road_.moveHorace(dir);
    }
}

void Game::setSteer(int dir) {
    if (state_ == GameState::Slope) {
        slope_.setSteer(dir);
    }
}

void Game::setTuck(bool tuck) {
    if (state_ == GameState::Slope) {
        slope_.setTuck(tuck);
    }
}

void Game::update(float dt) {
    switch (state_) {
    case GameState::Road:     updateRoad(dt);  break;
    case GameState::Slope:    updateSlope(dt); break;
    case GameState::GameOver: break;
    }
}

void Game::updateRoad(float dt) {
    switch (road_.update(dt)) {
    case RoadEvent::HitByCar:      break;           // скорая уже едет; тут можно проиграть звук удара
    case RoadEvent::AmbulanceLeft: payAmbulance(); break;  // скорая уехала — платим по счёту
    case RoadEvent::ReachedShop:   rentSkis();     break;
    case RoadEvent::ReachedSlope:  startSlope();   break;
    case RoadEvent::None:          break;
    }
}

void Game::updateSlope(float dt) {
    for (SlopeEvent event : slope_.update(dt)) {
        switch (event) {
        case SlopeEvent::PassedGate:
            score_ += kGatePoints;
            break;
        case SlopeEvent::Finished:
            score_ += kFinishBonus;
            finishRun(false);
            break;
        case SlopeEvent::SkisBroken:
            finishRun(true);
            break;
        case SlopeEvent::MissedGate:
        case SlopeEvent::HitTree:
            break;  // очков нет; урон лыжам SlopeLevel считает сам
        }
    }
}

void Game::startSlope() {
    state_ = GameState::Slope;
    slope_.startRun(nextCourseSeed_++);  // каждый заезд — новая трасса
}

void Game::finishRun(bool skisBroken) {
    if (state_ != GameState::Slope) {
        return;
    }
    ++runsCompleted_;
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
