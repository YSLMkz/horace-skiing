#pragma once

#include "core/Rect.hpp"

#include <vector>

namespace horace {

// Ворота: два флажка на высоте y. Проехать надо между leftX и rightX.
struct Gate {
    float y = 0.f;
    float leftX = 0.f;
    float rightX = 0.f;
    bool resolved = false;  // лыжник уже пересёк линию ворот
    bool passed = false;    // и при этом проехал между флажками
};

// Дерево. hit — в него уже врезались (второй раз урон не считаем).
struct Tree {
    Rect bounds;
    bool hit = false;
};

// Что случилось на склоне за кадр. За один кадр может случиться несколько
// событий сразу (проехал ворота и тут же врезался в дерево), поэтому
// update() возвращает список.
enum class SlopeEvent {
    PassedGate,  // проехал между флажками
    MissedGate,  // объехал ворота стороной
    HitTree,     // врезался в дерево — лыжи повреждены
    SkisBroken,  // лыжи сломаны — заезд окончен
    Finished,    // доехал до финиша
};

// Уровень «Склон».
//
// Координаты «мировые»: трасса — длинная вертикальная полоса шириной 800 px,
// y растёт вниз. Лыжник едет вниз по трассе, а камера едет за ним, так что
// на экране он всегда на одной высоте (kSkierScreenY), а трасса «едет» вверх.
class SlopeLevel {
public:
    static constexpr float kWidth        = 800.f;
    static constexpr float kScreenHeight = 600.f;
    static constexpr float kSkierScreenY = 150.f;   // где лыжник на экране
    static constexpr float kSkierWidth   = 30.f;
    static constexpr float kSkierHeight  = 40.f;
    static constexpr float kDownSpeed    = 220.f;   // обычная скорость вниз, px/s
    static constexpr float kTuckSpeed    = 380.f;   // со стрелкой вниз — быстрее
    static constexpr float kSideSpeed    = 260.f;   // скорость поворота
    static constexpr float kCrashTime    = 0.8f;    // сколько секунд лежим после дерева
    static constexpr int   kMaxDamage    = 3;       // столько деревьев — и лыжи сломаны
    static constexpr float kGateWidth    = 160.f;   // расстояние между флажками
    static constexpr float kGateSpacing  = 450.f;   // расстояние между воротами
    static constexpr int   kGateCount    = 15;
    static constexpr float kTreeWidth    = 30.f;
    static constexpr float kTreeHeight   = 40.f;

    SlopeLevel();

    // Новый заезд со случайной трассой. Одинаковый seed — одинаковая трасса.
    void startRun(unsigned seed);
    // Новый заезд по пустой трассе заданной длины (для тестов и своих трасс)
    void startEmptyRun(float courseLength);
    void addGate(float y, float centerX);
    void addTree(float x, float y);

    // Управление: dir = -1 влево, 0 прямо, +1 вправо; tuck — ускориться
    void setSteer(int dir) { steer_ = dir; }
    void setTuck(bool tuck) { tuck_ = tuck; }

    std::vector<SlopeEvent> update(float dt);

    // Демо-режим: деревья не мешают
    void setInvincible(bool on) { invincible_ = on; }
    bool isInvincible() const { return invincible_; }

    const Rect& skier() const { return skier_; }               // в мировых координатах
    float cameraY() const { return skier_.y - kSkierScreenY; } // мировой y верхнего края экрана
    const std::vector<Gate>& gates() const { return gates_; }
    const std::vector<Tree>& trees() const { return trees_; }
    float courseLength() const { return courseLength_; }       // где финиш
    int damage() const { return damage_; }
    bool isCrashed() const { return crashTimer_ > 0.f; }
    bool isOver() const { return over_; }

private:
    void resetSkier();
    void checkGates(float prevBottom, std::vector<SlopeEvent>& events);
    void checkTrees(std::vector<SlopeEvent>& events);

    Rect skier_;
    std::vector<Gate> gates_;
    std::vector<Tree> trees_;
    float courseLength_ = 0.f;
    int steer_ = 0;
    bool tuck_ = false;
    int damage_ = 0;
    float crashTimer_ = 0.f;
    bool over_ = false;
    bool invincible_ = false;
};

} // namespace horace
