#pragma once

#include <cstdint>
#include <vector>

namespace horace {

// «Бипер» в духе ZX Spectrum.
//
// У Спектрума не было звуковой карты: процессор просто переключал динамик
// между двумя положениями. Громкости нет — только «вкл/выкл», поэтому
// звук получается резкий, квадратный, «кислотный».
// Здесь мы делаем то же самое: каждый отсчёт звука равен либо +kLevel,
// либо -kLevel (или 0 — тишина). Никаких файлов: звуки синтезируются в коде.
namespace beeper {

using Samples = std::vector<std::int16_t>;

constexpr int kSampleRate = 44100;          // отсчётов в секунду
constexpr std::int16_t kLevel = 6000;       // «громкость» единицы (из 32767 возможных)

// Квадратная волна, частота плавно меняется от fromHz до toHz.
// duty — какую долю периода динамик «включён» (0.5 — обычный писк,
// 0.1 — тонкий противный зуммер).
Samples square(float fromHz, float toHz, float seconds, float duty = 0.5f);

// Квадратная волна с вибрато: частота качается вокруг centerHz
// на ±depthHz rateHz раз в секунду. Это и есть «плывущий» звук.
Samples vibrato(float centerHz, float depthHz, float rateHz, float seconds, float duty = 0.5f);

// «Аккорд» на одном динамике: очень быстро перебираем ноты по кругу
// (switchHz раз в секунду). Так на Спектруме изображали аккорды.
Samples chord(const std::vector<float>& notesHz, float seconds, float switchHz);

// 1-битный шум: динамик дёргается случайно, rateHz раз в секунду.
// Одинаковый seed — одинаковый шум.
Samples noise(float seconds, float rateHz, unsigned seed);

Samples silence(float seconds);

// Дописать from в конец to
void append(Samples& to, const Samples& from);

// Звуки игры
enum class Sound {
    Step,        // шаг по дороге
    Crash,       // сбила машина
    Siren,       // сирена скорой (играет по кругу)
    SkisRented,  // взял лыжи — касса
    GatePassed,  // проехал ворота
    GateMissed,  // пропустил ворота — противный зуммер
    TreeHit,     // врезался в дерево
    SkisBroken,  // сломал лыжи
    Finished,    // финиш — фанфары
    GameOver,    // конец игры
    Count        // сколько всего звуков (не звук)
};

Samples makeSound(Sound sound);

} // namespace beeper
} // namespace horace
