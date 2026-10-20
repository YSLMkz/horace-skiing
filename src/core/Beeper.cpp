#include "core/Beeper.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <random>

namespace horace {
namespace beeper {

namespace {

constexpr double kPi = 3.14159265358979323846;

int sampleCount(float seconds) {
    return std::max(0, static_cast<int>(std::lround(seconds * kSampleRate)));
}

// Основа всех звуков: квадратная волна, частота которой в момент t
// задаётся функцией freqAt(t). Фаза копится, поэтому при смене частоты
// нет щелчков — волна просто начинает «бежать» быстрее или медленнее.
Samples synth(float seconds, float duty, const std::function<double(double)>& freqAt) {
    Samples out(static_cast<size_t>(sampleCount(seconds)));
    double phase = 0.0;
    for (size_t i = 0; i < out.size(); ++i) {
        const double t = static_cast<double>(i) / kSampleRate;
        phase += freqAt(t) / kSampleRate;
        const double frac = phase - std::floor(phase);
        out[i] = frac < duty ? kLevel : static_cast<std::int16_t>(-kLevel);
    }
    return out;
}

// Плавный переход частоты «по нотам» (экспоненциально), как слышит ухо
double glide(double fromHz, double toHz, double t, double total) {
    if (total <= 0.0 || fromHz <= 0.0 || toHz <= 0.0) {
        return fromHz;
    }
    return fromHz * std::pow(toHz / fromHz, t / total);
}

// Ноты (Гц)
constexpr float C5 = 523.25f, E5 = 659.25f, G5 = 783.99f;
constexpr float C6 = 1046.5f, E6 = 1318.5f, G6 = 1567.98f, C7 = 2093.0f;

} // namespace

Samples square(float fromHz, float toHz, float seconds, float duty) {
    return synth(seconds, duty, [=](double t) { return glide(fromHz, toHz, t, seconds); });
}

Samples vibrato(float centerHz, float depthHz, float rateHz, float seconds, float duty) {
    return synth(seconds, duty, [=](double t) {
        return centerHz + depthHz * std::sin(2.0 * kPi * rateHz * t);
    });
}

Samples chord(const std::vector<float>& notesHz, float seconds, float switchHz) {
    if (notesHz.empty()) {
        return silence(seconds);
    }
    return synth(seconds, 0.5f, [&notesHz, switchHz](double t) {
        const auto index = static_cast<size_t>(t * switchHz) % notesHz.size();
        return static_cast<double>(notesHz[index]);
    });
}

Samples noise(float seconds, float rateHz, unsigned seed) {
    Samples out(static_cast<size_t>(sampleCount(seconds)));
    std::mt19937 rng(seed);  // mt19937 выдаёт одинаковые числа на любом компьютере
    const int hold = std::max(1, static_cast<int>(kSampleRate / std::max(1.f, rateHz)));
    std::int16_t value = kLevel;
    for (size_t i = 0; i < out.size(); ++i) {
        if (i % static_cast<size_t>(hold) == 0) {
            value = (rng() & 1u) ? kLevel : static_cast<std::int16_t>(-kLevel);
        }
        out[i] = value;
    }
    return out;
}

Samples silence(float seconds) {
    return Samples(static_cast<size_t>(sampleCount(seconds)), 0);
}

void append(Samples& to, const Samples& from) {
    to.insert(to.end(), from.begin(), from.end());
}

Samples makeSound(Sound sound) {
    Samples s;
    switch (sound) {
    case Sound::Step:
        // короткий «тик» вверх
        s = square(1800, 2600, 0.015f);
        break;

    case Sound::Crash:
        // треск, падение тона и «рассыпающийся» шум
        s = noise(0.25f, 4000, 1);
        append(s, square(500, 60, 0.25f, 0.3f));
        append(s, noise(0.2f, 900, 2));
        break;

    case Sound::Siren:
        // «ии-уу» с дрожащим тоном; играет по кругу, пока едет скорая
        for (int i = 0; i < 2; ++i) {
            append(s, vibrato(960, 30, 25, 0.28f));
            append(s, vibrato(720, 30, 25, 0.28f));
        }
        break;

    case Sound::SkisRented:
        // касса: «трынь-трынь» вверх и звон
        for (int i = 0; i < 2; ++i) {
            for (float note : {C6, E6, G6, C7}) {
                append(s, square(note, note, 0.035f));
            }
        }
        append(s, noise(0.06f, 9000, 7));
        break;

    case Sound::GatePassed:
        // «вжик» вверх
        s = square(880, 1760, 0.05f);
        append(s, square(1760, 1760, 0.04f));
        break;

    case Sound::GateMissed:
        // в оригинале за пропуск ворот — предупреждающий сигнал. Бз-бз-бз-бззз
        for (int i = 0; i < 3; ++i) {
            append(s, square(150, 150, 0.11f, 0.08f));
            append(s, silence(0.04f));
        }
        append(s, square(100, 90, 0.15f, 0.08f));
        break;

    case Sound::TreeHit:
        s = noise(0.08f, 2500, 3);
        append(s, square(320, 70, 0.15f, 0.2f));
        break;

    case Sound::SkisBroken:
        // долгий «пьяный» спуск вниз с сильным вибрато
        s = synth(0.9f, 0.5f, [](double t) {
            return glide(1200, 100, t, 0.9) + 60.0 * std::sin(2.0 * kPi * 18.0 * t);
        });
        break;

    case Sound::Finished:
        // фанфары: аккорды-трели всё выше, потом визг вверх
        for (float octave : {1.f, 2.f, 4.f}) {
            append(s, chord({C5 * octave, E5 * octave, G5 * octave}, 0.15f, 60));
        }
        append(s, chord({G6, C7}, 0.5f, 20));
        append(s, square(C7, 2 * C7, 0.1f));
        break;

    case Sound::GameOver: {
        // грустная хроматическая лесенка вниз, в конце — дрожащий бас
        float note = C5;
        for (int i = 0; i < 8; ++i) {
            append(s, vibrato(note, 20, 30, 0.11f));
            note /= 1.0594631f;  // на полтона ниже
        }
        append(s, vibrato(110, 30, 8, 0.7f, 0.3f));
        break;
    }

    case Sound::BonusMoney:
        // «монетка» три раза подряд, каждый раз выше, и искрящийся аккорд
        for (float k : {1.f, 1.5f, 2.f}) {
            append(s, square(988 * k, 988 * k, 0.05f));
            append(s, square(1319 * k, 1319 * k, 0.12f));
        }
        append(s, chord({C7, E6 * 2, G6 * 2}, 0.3f, 50));
        break;

    case Sound::Count:
        break;
    }
    return s;
}

} // namespace beeper
} // namespace horace
