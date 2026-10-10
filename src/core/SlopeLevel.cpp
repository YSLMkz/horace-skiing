#include "core/SlopeLevel.hpp"

#include <algorithm>
#include <random>

namespace horace {

SlopeLevel::SlopeLevel() {
    startEmptyRun(kScreenHeight);
}

void SlopeLevel::resetSkier() {
    skier_ = Rect{kWidth / 2.f - kSkierWidth / 2.f, 0.f, kSkierWidth, kSkierHeight};
    steer_ = 0;
    tuck_ = false;
    damage_ = 0;
    crashTimer_ = 0.f;
    over_ = false;
}

void SlopeLevel::startEmptyRun(float courseLength) {
    gates_.clear();
    trees_.clear();
    courseLength_ = courseLength;
    resetSkier();
}

void SlopeLevel::addGate(float y, float centerX) {
    gates_.push_back(Gate{y, centerX - kGateWidth / 2.f, centerX + kGateWidth / 2.f});
}

void SlopeLevel::addTree(float x, float y) {
    trees_.push_back(Tree{Rect{x, y, kTreeWidth, kTreeHeight}});
}

void SlopeLevel::startRun(unsigned seed) {
    // Генератор с seed: при одинаковом seed трасса всегда одинаковая —
    // это удобно для отладки и тестов.
    std::mt19937 rng(seed);
    const float margin = kGateWidth / 2.f + 40.f;
    std::uniform_real_distribution<float> gateX(margin, kWidth - margin);
    std::uniform_real_distribution<float> treeX(0.f, kWidth - kTreeWidth);

    const float firstGateY = 600.f;
    startEmptyRun(firstGateY + kGateCount * kGateSpacing + 200.f);

    for (int i = 0; i < kGateCount; ++i) {
        addGate(firstGateY + i * kGateSpacing, gateX(rng));
    }

    // Деревья — между воротами, не ближе 120 px к линии ворот,
    // чтобы ворота всегда можно было проехать.
    std::uniform_real_distribution<float> treeOffset(120.f, kGateSpacing - 120.f - kTreeHeight);
    for (int i = 0; i < kGateCount; ++i) {
        const float segmentTop = firstGateY + i * kGateSpacing;
        for (int t = 0; t < 3; ++t) {
            addTree(treeX(rng), segmentTop + treeOffset(rng));
        }
    }
}

std::vector<SlopeEvent> SlopeLevel::update(float dt) {
    std::vector<SlopeEvent> events;
    if (over_) {
        return events;
    }

    // После удара о дерево лыжник немного лежит и не двигается
    if (crashTimer_ > 0.f) {
        crashTimer_ = std::max(0.f, crashTimer_ - dt);
        return events;
    }

    const float prevBottom = skier_.y + skier_.height;

    skier_.y += (tuck_ ? kTuckSpeed : kDownSpeed) * dt;
    skier_.x += static_cast<float>(steer_) * kSideSpeed * dt;
    skier_.x = std::clamp(skier_.x, 0.f, kWidth - skier_.width);

    checkGates(prevBottom, events);
    checkTrees(events);

    if (over_) {
        return events;  // лыжи сломались — до финиша уже не доехали
    }
    if (skier_.y + skier_.height >= courseLength_) {
        over_ = true;
        events.push_back(SlopeEvent::Finished);
    }
    return events;
}

void SlopeLevel::checkGates(float prevBottom, std::vector<SlopeEvent>& events) {
    const float bottom = skier_.y + skier_.height;
    const float centerX = skier_.x + skier_.width / 2.f;

    for (Gate& gate : gates_) {
        // Ворота считаются, когда низ лыжника пересёк линию ворот за этот кадр
        if (gate.resolved || prevBottom >= gate.y || bottom < gate.y) {
            continue;
        }
        gate.resolved = true;
        gate.passed = centerX > gate.leftX && centerX < gate.rightX;
        events.push_back(gate.passed ? SlopeEvent::PassedGate : SlopeEvent::MissedGate);
    }
}

void SlopeLevel::checkTrees(std::vector<SlopeEvent>& events) {
    for (Tree& tree : trees_) {
        if (tree.hit || !tree.bounds.intersects(skier_)) {
            continue;
        }
        tree.hit = true;
        ++damage_;
        crashTimer_ = kCrashTime;
        events.push_back(SlopeEvent::HitTree);

        if (damage_ >= kMaxDamage) {
            over_ = true;
            events.push_back(SlopeEvent::SkisBroken);
        }
        return;  // за один кадр — не больше одного дерева
    }
}

} // namespace horace
