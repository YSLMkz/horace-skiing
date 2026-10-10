#include "core/SlopeLevel.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

using horace::SlopeEvent;
using horace::SlopeLevel;

namespace {

bool contains(const std::vector<SlopeEvent>& events, SlopeEvent e) {
    return std::find(events.begin(), events.end(), e) != events.end();
}

// Едет вниз маленькими шагами, пока не случится нужное событие (или не кончится время)
bool rideUntil(SlopeLevel& slope, SlopeEvent wanted, float maxSeconds = 60.f) {
    for (float t = 0; t < maxSeconds; t += 0.01f) {
        if (contains(slope.update(0.01f), wanted)) {
            return true;
        }
    }
    return false;
}

float skierCenterX(const SlopeLevel& slope) {
    return slope.skier().x + slope.skier().width / 2.f;
}

} // namespace

TEST(SlopeLevelTest, SkierMovesDown) {
    SlopeLevel slope;
    slope.startEmptyRun(10000);
    const float y0 = slope.skier().y;
    slope.update(1.f);
    EXPECT_FLOAT_EQ(slope.skier().y, y0 + SlopeLevel::kDownSpeed);
}

TEST(SlopeLevelTest, TuckIsFaster) {
    SlopeLevel slope;
    slope.startEmptyRun(10000);
    const float y0 = slope.skier().y;
    slope.setTuck(true);
    slope.update(1.f);
    EXPECT_FLOAT_EQ(slope.skier().y, y0 + SlopeLevel::kTuckSpeed);
}

TEST(SlopeLevelTest, SteeringMovesSideways) {
    SlopeLevel slope;
    slope.startEmptyRun(10000);
    const float x0 = slope.skier().x;
    slope.setSteer(-1);
    slope.update(0.5f);
    EXPECT_FLOAT_EQ(slope.skier().x, x0 - SlopeLevel::kSideSpeed * 0.5f);
}

TEST(SlopeLevelTest, CannotLeaveCourse) {
    SlopeLevel slope;
    slope.startEmptyRun(100000);
    slope.setSteer(1);
    slope.update(10.f);
    EXPECT_FLOAT_EQ(slope.skier().x, SlopeLevel::kWidth - slope.skier().width);
}

TEST(SlopeLevelTest, CameraFollowsSkier) {
    SlopeLevel slope;
    slope.startEmptyRun(10000);
    slope.update(2.f);
    EXPECT_FLOAT_EQ(slope.skier().y - slope.cameraY(), SlopeLevel::kSkierScreenY);
}

TEST(SlopeLevelTest, GateRightInFrontIsPassed) {
    SlopeLevel slope;
    slope.startEmptyRun(10000);
    slope.addGate(300, skierCenterX(slope));  // ворота точно по курсу
    EXPECT_TRUE(rideUntil(slope, SlopeEvent::PassedGate));
    EXPECT_TRUE(slope.gates()[0].passed);
}

TEST(SlopeLevelTest, GateToTheSideIsMissed) {
    SlopeLevel slope;
    slope.startEmptyRun(10000);
    slope.addGate(300, skierCenterX(slope) + 300);  // ворота сильно правее
    EXPECT_TRUE(rideUntil(slope, SlopeEvent::MissedGate));
    EXPECT_FALSE(slope.gates()[0].passed);
}

TEST(SlopeLevelTest, GateIsCountedOnlyOnce) {
    SlopeLevel slope;
    slope.startEmptyRun(10000);
    slope.addGate(300, skierCenterX(slope));
    int passed = 0;
    for (int i = 0; i < 500; ++i) {
        for (SlopeEvent e : slope.update(0.01f)) {
            if (e == SlopeEvent::PassedGate) ++passed;
        }
    }
    EXPECT_EQ(passed, 1);
}

TEST(SlopeLevelTest, TreeDamagesSkisAndStopsSkier) {
    SlopeLevel slope;
    slope.startEmptyRun(10000);
    slope.addTree(slope.skier().x, 300);  // дерево точно по курсу
    EXPECT_TRUE(rideUntil(slope, SlopeEvent::HitTree));
    EXPECT_EQ(slope.damage(), 1);
    EXPECT_TRUE(slope.isCrashed());

    const float y = slope.skier().y;
    slope.update(SlopeLevel::kCrashTime / 2);  // лежим — не едем
    EXPECT_FLOAT_EQ(slope.skier().y, y);
}

TEST(SlopeLevelTest, SkierGetsUpAfterCrash) {
    SlopeLevel slope;
    slope.startEmptyRun(10000);
    slope.addTree(slope.skier().x, 300);
    rideUntil(slope, SlopeEvent::HitTree);
    slope.update(SlopeLevel::kCrashTime + 0.01f);
    EXPECT_FALSE(slope.isCrashed());
    const float y = slope.skier().y;
    slope.update(0.1f);
    EXPECT_GT(slope.skier().y, y);
}

TEST(SlopeLevelTest, SameTreeHurtsOnlyOnce) {
    SlopeLevel slope;
    slope.startEmptyRun(10000);
    slope.addTree(slope.skier().x, 300);
    rideUntil(slope, SlopeEvent::HitTree);
    rideUntil(slope, SlopeEvent::HitTree, 3.f);  // проезжаем дальше
    EXPECT_EQ(slope.damage(), 1);
}

TEST(SlopeLevelTest, TooManyTreesBreakSkis) {
    SlopeLevel slope;
    slope.startEmptyRun(10000);
    for (int i = 0; i < SlopeLevel::kMaxDamage; ++i) {
        slope.addTree(slope.skier().x, 300 + i * 200.f);
    }
    EXPECT_TRUE(rideUntil(slope, SlopeEvent::SkisBroken));
    EXPECT_TRUE(slope.isOver());
}

TEST(SlopeLevelTest, ReachingFinishEndsRun) {
    SlopeLevel slope;
    slope.startEmptyRun(1000);
    EXPECT_TRUE(rideUntil(slope, SlopeEvent::Finished));
    EXPECT_TRUE(slope.isOver());
    EXPECT_TRUE(slope.update(1.f).empty());  // после финиша ничего не происходит
}

TEST(SlopeLevelTest, GeneratedCourseHasGatesTreesAndFinishAfterLastGate) {
    SlopeLevel slope;
    slope.startRun(42);
    ASSERT_EQ(static_cast<int>(slope.gates().size()), SlopeLevel::kGateCount);
    EXPECT_FALSE(slope.trees().empty());
    EXPECT_GT(slope.courseLength(), slope.gates().back().y);
}

TEST(SlopeLevelTest, SameSeedGivesSameCourse) {
    SlopeLevel a, b;
    a.startRun(7);
    b.startRun(7);
    ASSERT_EQ(a.gates().size(), b.gates().size());
    for (size_t i = 0; i < a.gates().size(); ++i) {
        EXPECT_FLOAT_EQ(a.gates()[i].leftX, b.gates()[i].leftX);
    }
}

TEST(SlopeLevelTest, GeneratedGatesFitOnCourse) {
    SlopeLevel slope;
    for (unsigned seed = 1; seed <= 20; ++seed) {
        slope.startRun(seed);
        for (const auto& gate : slope.gates()) {
            EXPECT_GE(gate.leftX, 0.f);
            EXPECT_LE(gate.rightX, SlopeLevel::kWidth);
        }
    }
}

TEST(SlopeLevelTest, GeneratedTreesDoNotBlockGateLines) {
    SlopeLevel slope;
    for (unsigned seed = 1; seed <= 20; ++seed) {
        slope.startRun(seed);
        for (const auto& tree : slope.trees()) {
            for (const auto& gate : slope.gates()) {
                const bool overlapsGateLine =
                    tree.bounds.y < gate.y + 50 && tree.bounds.y + tree.bounds.height > gate.y - 50;
                EXPECT_FALSE(overlapsGateLine) << "seed " << seed;
            }
        }
    }
}
