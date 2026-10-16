#pragma once

#include "core/Beeper.hpp"
#include "core/Game.hpp"

#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/SoundBuffer.hpp>

#include <array>
#include <vector>

namespace horace {

// Проигрывает звуки бипера по событиям игры.
// Звуки синтезируются один раз при запуске (beeper::makeSound) и хранятся в памяти.
//
// Важно: sf::Sound хранит указатель на свой sf::SoundBuffer, поэтому SoundPlayer
// нельзя копировать — создайте один раз в main и не перемещайте.
class SoundPlayer {
public:
    SoundPlayer();
    SoundPlayer(const SoundPlayer&) = delete;
    SoundPlayer& operator=(const SoundPlayer&) = delete;

    // Проиграть звуки для всех событий за кадр
    void handle(const std::vector<GameEvent>& events);
    // Сирена играет по кругу, пока on == true
    void setSiren(bool on);

    void toggleMute();
    bool isMuted() const { return muted_; }

private:
    void play(beeper::Sound sound);

    static constexpr int kVoices = 8;  // сколько звуков может звучать одновременно
    static constexpr float kVolume = 60.f;  // 0..100

    std::array<sf::SoundBuffer, static_cast<size_t>(beeper::Sound::Count)> buffers_;
    std::array<sf::Sound, kVoices> voices_;
    size_t nextVoice_ = 0;
    sf::Sound siren_;
    bool muted_ = false;
};

} // namespace horace
