#include "core/Ambulance.hpp"

#include <gtest/gtest.h>

using horace::Ambulance;
using horace::AmbulancePhase;
using horace::Rect;

namespace {
const Rect kPatient{400, 200, 40, 40};

// Крутит скорую, пока не наступит нужный этап (не дольше 10 секунд)
bool runUntilPhase(Ambulance& a, AmbulancePhase phase) {
    for (int i = 0; i < 1000 && a.phase() != phase; ++i) {
        a.update(0.01f);
    }
    return a.phase() == phase;
}
} // namespace

TEST(AmbulanceTest, IdleByDefault) {
    Ambulance a;
    EXPECT_FALSE(a.isActive());
    EXPECT_FALSE(a.update(1.f));
}

TEST(AmbulanceTest, ArrivesFromLeftInPatientsRow) {
    Ambulance a;
    a.call(kPatient, 800);
    EXPECT_EQ(a.phase(), AmbulancePhase::Arriving);
    EXPECT_LT(a.bounds().x, 0.f);                 // за левым краем
    EXPECT_FLOAT_EQ(a.bounds().y, kPatient.y);    // на той же полосе
}

TEST(AmbulanceTest, StopsRightNextToPatient) {
    Ambulance a;
    a.call(kPatient, 800);
    ASSERT_TRUE(runUntilPhase(a, AmbulancePhase::Loading));
    EXPECT_FLOAT_EQ(a.bounds().x + a.bounds().width, kPatient.x);  // бампер упёрся в Хораса
}

TEST(AmbulanceTest, WaitsWhileLoading) {
    Ambulance a;
    a.call(kPatient, 800);
    runUntilPhase(a, AmbulancePhase::Loading);
    const float x = a.bounds().x;
    a.update(Ambulance::kLoadingTime / 2);
    EXPECT_EQ(a.phase(), AmbulancePhase::Loading);
    EXPECT_FLOAT_EQ(a.bounds().x, x);
}

TEST(AmbulanceTest, LeavesAfterLoading) {
    Ambulance a;
    a.call(kPatient, 800);
    runUntilPhase(a, AmbulancePhase::Loading);
    a.update(Ambulance::kLoadingTime + 0.01f);
    EXPECT_EQ(a.phase(), AmbulancePhase::Leaving);
}

TEST(AmbulanceTest, ReportsOnceWhenGone) {
    Ambulance a;
    a.call(kPatient, 800);
    int reports = 0;
    for (int i = 0; i < 1000; ++i) {
        if (a.update(0.01f)) ++reports;
    }
    EXPECT_EQ(reports, 1);
    EXPECT_FALSE(a.isActive());
}

TEST(AmbulanceTest, PatientAtLeftEdgeIsReachedImmediately) {
    Ambulance a;
    a.call(Rect{0, 200, 40, 40}, 800);
    a.update(0.01f);
    EXPECT_EQ(a.phase(), AmbulancePhase::Loading);
}
