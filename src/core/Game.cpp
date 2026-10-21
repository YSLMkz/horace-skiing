#include "core/Game.hpp"

#include <utility>

namespace horace {

Game::Game() = default;

void Game::confirm() {
    switch (state_) {
    case GameState::Menu:
        state_ = GameState::Road;
        road_.startFromBottom();  // пока было меню, по дороге просто ездили машины
        emit(GameEvent::Confirm);
        break;
    case GameState::RunResults:
        continueAfterResults();
        emit(GameEvent::Confirm);
        break;
    case GameState::EnterName:
        saveHighScore();
        emit(GameEvent::Confirm);
        break;
    case GameState::GameOver: {
        // Всё с нуля — снова стартовое меню. Но рекорды и демо-режим сохраняем!
        HighScores keep = std::move(highScores_);
        const bool demo = demoMode_;
        *this = Game();
        highScores_ = std::move(keep);
        applyDemoMode(demo);
        emit(GameEvent::Confirm);
        break;
    }
    case GameState::Road:
    case GameState::Slope:
        break;  // во время игры Enter ничего не делает
    }
}

std::vector<GameEvent> Game::takeEvents() {
    return std::exchange(events_, {});  // отдать список и оставить пустой
}

void Game::moveHorace(Direction dir) {
    if (state_ == GameState::Road && !paused_ && road_.moveHorace(dir)) {
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

void Game::togglePause() {
    if (state_ == GameState::Road || state_ == GameState::Slope) {
        paused_ = !paused_;
    }
}

void Game::addScore(int points) {
    score_ += points;
    if (score_ < 0) {
        score_ = 0;
    }
    // Порог только растёт: если очки отняли штрафом и снова набрали,
    // за ту же тысячу второй раз не платим
    while (score_ >= nextCashAt_) {
        money_ += kCashBonus;
        nextCashAt_ += kPointsForCash;
        emit(GameEvent::BonusMoney);
    }
}

void Game::pressDemoKey() {
    // Слишком долгая пауза между нажатиями — считаем заново
    if (sinceDemoPress_ > kDemoPressGap) {
        demoPresses_ = 0;
    }
    sinceDemoPress_ = 0.f;
    if (++demoPresses_ < kDemoPresses) {
        return;
    }
    demoPresses_ = 0;
    applyDemoMode(!demoMode_);
    emit(demoMode_ ? GameEvent::DemoModeOn : GameEvent::DemoModeOff);
}

void Game::applyDemoMode(bool on) {
    demoMode_ = on;
    road_.setInvincible(on);
    slope_.setInvincible(on);
}

void Game::update(float dt) {
    sinceDemoPress_ += dt;  // считаем и на паузе, и в меню
    if (paused_) {
        return;
    }
    switch (state_) {
    case GameState::Menu:
        road_.update(dt);  // фон меню: по дороге ездят машины (Хорас стоит на тротуаре)
        break;
    case GameState::Road:
        updateRoad(dt);
        break;
    case GameState::Slope:
        updateSlope(dt);
        break;
    case GameState::RunResults:
    case GameState::EnterName:
    case GameState::GameOver:
        break;
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
            ++run_.gatesPassed;
            emit(GameEvent::GatePassed);
            addScore(kGatePoints);
            break;
        case SlopeEvent::MissedGate:
            ++run_.gatesMissed;
            emit(GameEvent::GateMissed);  // в оригинале — предупреждающий сигнал и штраф
            addScore(-kGatePenalty);
            break;
        case SlopeEvent::HitTree:
            ++run_.treesHit;
            emit(GameEvent::TreeHit);     // урон лыжам SlopeLevel считает сам
            break;
        case SlopeEvent::Finished:
            run_.finished = true;
            emit(GameEvent::Finished);
            addScore(kFinishBonus);
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
    run_ = RunStats{};
    runStartScore_ = score_;
    runStartMoney_ = money_;
}

void Game::finishRun(bool skisBroken) {
    if (state_ != GameState::Slope) {
        return;
    }
    ++runsCompleted_;
    run_.number = runsCompleted_;
    run_.skisBroken = skisBroken;
    run_.points = score_ - runStartScore_;
    run_.cashBonus = money_ - runStartMoney_;
    lastRun_ = run_;
    paused_ = false;
    state_ = GameState::RunResults;
}

void Game::continueAfterResults() {
    state_ = GameState::Road;
    road_.placeAtShop();
    if (!lastRun_.skisBroken) {
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
    paused_ = false;
    lastRank_ = -1;
    // Попал в таблицу рекордов — сначала спросим имя
    state_ = highScores_.qualifies(score_) ? GameState::EnterName : GameState::GameOver;
    nameInput_.clear();
    emit(GameEvent::GameOver);
}

void Game::typeNameChar(char32_t c) {
    if (state_ != GameState::EnterName) {
        return;
    }
    if (c == 8) {  // Backspace
        utf8::popBack(nameInput_);
        return;
    }
    const bool latin = (c >= U'A' && c <= U'Z') || (c >= U'a' && c <= U'z');
    const bool digit = c >= U'0' && c <= U'9';
    const bool cyrillic = (c >= 0x0410 && c <= 0x044F) || c == 0x0401 || c == 0x0451;  // А-я, Ё, ё
    const bool other = c == U' ' || c == U'-' || c == U'_';
    if ((latin || digit || cyrillic || other) && utf8::length(nameInput_) < HighScores::kMaxNameLength) {
        utf8::append(nameInput_, c);
    }
}

void Game::saveHighScore() {
    lastRank_ = highScores_.add(ScoreEntry{nameInput_, score_, runsCompleted_});
    state_ = GameState::GameOver;
    emit(GameEvent::HighScoreAdded);
}

} // namespace horace
