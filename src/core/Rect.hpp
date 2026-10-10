#pragma once

namespace horace {

// Прямоугольник на игровом поле: левый верхний угол (x, y), ширина и высота.
// Всё в игре (Хорас, машины, флажки, деревья) описывается такими прямоугольниками.
struct Rect {
    float x = 0.f;
    float y = 0.f;
    float width = 0.f;
    float height = 0.f;

    // true, если прямоугольники перекрываются (касание краями не считается)
    bool intersects(const Rect& other) const;
};

} // namespace horace
