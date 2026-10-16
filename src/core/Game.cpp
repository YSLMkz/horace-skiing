#include "core/Game.hpp"

#include <utility>

namespace horace {

Game::Game() = default;

std::vector<GameEvent> Game::takeEvents() {
    return std::exchange(events_, {});  // отдать список и оставить пустой
}

void Game::moveHorace(Direction dir) {
    if (state_ == GameState::Road && road_.moveHorace(dir)) {
        emit(GameEvent::Step);
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
    case RoadEvent::HitByCar:      emit(GameEvent::Crash); break;  // скорая уже едет
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
            emit(GameEvent::GatePassed);
            break;
        case SlopeEvent::MissedGate:
            emit(GameEvent::GateMissed);  // TODO: по правилам оригинала ещё и штраф очками
            break;
        case SlopeEvent::HitTree:
            emit(GameEvent::TreeHit);     // урон лыжам SlopeLevel считает сам
            break;
        case SlopeEvent::Finished:
            score_ += kFinishBonus;
            emit(GameEvent::Finished);
            finishRun(false);
            break;
        case SlopeEvent::SkisBroken:
            emit(GameEvent::SkisBroken);
            finishRun(true);
            break;
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
    emit(GameEvent::SkisRented);
}

void Game::gameOver() {
    money_ = 0;
    state_ = GameState::GameOver;
    emit(GameEvent::GameOver);
}

} // namespace horace
