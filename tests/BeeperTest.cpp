#include "core/Beeper.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>

using namespace horace::beeper;

namespace {

// Сколько раз волна «перепрыгнула» снизу вверх — это и есть число периодов
int risingEdges(const Samples& s, size_t from = 0, size_t to = SIZE_MAX) {
    to = std::min(to, s.size());
    int edges = 0;
    for (size_t i = from + 1; i < to; ++i) {
        if (s[i - 1] < 0 && s[i] > 0) ++edges;
    }
    return edges;
}

bool isOneBit(const Samples& s) {
    for (auto v : s) {
        if (v != kLevel && v != -kLevel && v != 0) return false;
    }
    return true;
}

} // namespace

TEST(BeeperTest, LengthMatchesSeconds) {
    EXPECT_EQ(square(440, 440, 0.5f).size(), static_cast<size_t>(kSampleRate / 2));
    EXPECT_EQ(silence(0.1f).size(), static_cast<size_t>(kSampleRate / 10));
}

TEST(BeeperTest, SquareHasRightFrequency) {
    const Samples s = square(440, 440, 1.f);
    EXPECT_NEAR(risingEdges(s), 440, 1);
}

TEST(BeeperTest, SweepUpGetsHigher) {
    const Samples s = square(200, 2000, 1.f);
    const size_t half = s.size() / 2;
    EXPECT_LT(risingEdges(s, 0, half), risingEdges(s, half));
}

TEST(BeeperTest, SweepDownGetsLower) {
    const Samples s = square(2000, 200, 1.f);
    const size_t half = s.size() / 2;
    EXPECT_GT(risingEdges(s, 0, half), risingEdges(s, half));
}

TEST(BeeperTest, SilenceIsZero) {
    for (auto v : silence(0.05f)) EXPECT_EQ(v, 0);
}

TEST(BeeperTest, NoiseIsRepeatableWithSameSeed) {
    EXPECT_EQ(noise(0.1f, 3000, 5), noise(0.1f, 3000, 5));
    EXPECT_NE(noise(0.1f, 3000, 5), noise(0.1f, 3000, 6));
}

TEST(BeeperTest, AppendJoinsSounds) {
    Samples a = square(440, 440, 0.1f);
    const size_t before = a.size();
    append(a, silence(0.1f));
    EXPECT_EQ(a.size(), before + silence(0.1f).size());
}

// Главное свойство бипера: у динамика только два положения (и тишина)
TEST(BeeperTest, AllGameSoundsAreOneBitAndNotTooLong) {
    for (int i = 0; i < static_cast<int>(Sound::Count); ++i) {
        const Samples s = makeSound(static_cast<Sound>(i));
        EXPECT_FALSE(s.empty()) << "sound " << i;
        EXPECT_TRUE(isOneBit(s)) << "sound " << i;
        EXPECT_LT(s.size(), static_cast<size_t>(3 * kSampleRate)) << "sound " << i;
    }
}

TEST(BeeperTest, ChordSwitchesBetweenNotes) {
    // аккорд из двух далёких нот: в первой и второй половине переключения
    // частота разная, а за всю секунду — примерно среднее
    const Samples s = chord({200, 2000}, 1.f, 2);  // 0.5 c — 200 Гц, 0.5 c — 2000 Гц
    const size_t half = s.size() / 2;
    EXPECT_NEAR(risingEdges(s, 0, half), 100, 2);
    EXPECT_NEAR(risingEdges(s, half), 1000, 2);
}
