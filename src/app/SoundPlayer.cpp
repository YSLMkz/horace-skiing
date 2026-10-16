#include "app/SoundPlayer.hpp"

namespace horace {

namespace {

// Какое событие игры каким звуком озвучивать
beeper::Sound soundFor(GameEvent event) {
    switch (event) {
    case GameEvent::Step:       return beeper::Sound::Step;
    case GameEvent::Crash:      return beeper::Sound::Crash;
    case GameEvent::SkisRented: return beeper::Sound::SkisRented;
    case GameEvent::GatePassed: return beeper::Sound::GatePassed;
    case GameEvent::GateMissed: return beeper::Sound::GateMissed;
    case GameEvent::TreeHit:    return beeper::Sound::TreeHit;
    case GameEvent::SkisBroken: return beeper::Sound::SkisBroken;
    case GameEvent::Finished:   return beeper::Sound::Finished;
    case GameEvent::GameOver:   return beeper::Sound::GameOver;
    }
    return beeper::Sound::Step;
}

} // namespace

SoundPlayer::SoundPlayer() {
    // Синтезируем все звуки и кладём их в звуковые буферы SFML
    for (size_t i = 0; i < buffers_.size(); ++i) {
        const beeper::Samples samples = beeper::makeSound(static_cast<beeper::Sound>(i));
        buffers_[i].loadFromSamples(samples.data(), samples.size(), 1 /* моно */, beeper::kSampleRate);
    }
    for (sf::Sound& voice : voices_) {
        voice.setVolume(kVolume);
    }
    siren_.setBuffer(buffers_[static_cast<size_t>(beeper::Sound::Siren)]);
    siren_.setLoop(true);
    siren_.setVolume(kVolume);
}

void SoundPlayer::handle(const std::vector<GameEvent>& events) {
    for (GameEvent event : events) {
        play(soundFor(event));
    }
}

void SoundPlayer::play(beeper::Sound sound) {
    if (muted_) {
        return;
    }
    // Берём «голоса» по кругу: если все заняты, самый старый звук обрывается
    sf::Sound& voice = voices_[nextVoice_];
    nextVoice_ = (nextVoice_ + 1) % voices_.size();
    voice.setBuffer(buffers_[static_cast<size_t>(sound)]);
    voice.play();
}

void SoundPlayer::setSiren(bool on) {
    const bool playing = siren_.getStatus() == sf::Sound::Playing;
    if (on && !muted_ && !playing) {
        siren_.play();
    } else if ((!on || muted_) && playing) {
        siren_.stop();
    }
}

void SoundPlayer::toggleMute() {
    muted_ = !muted_;
    if (muted_) {
        for (sf::Sound& voice : voices_) {
            voice.stop();
        }
        siren_.stop();
    }
}

} // namespace horace
