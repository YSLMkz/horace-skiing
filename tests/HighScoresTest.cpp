#include "core/HighScores.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <sstream>
#include <string>

using horace::HighScores;
using horace::ScoreEntry;

namespace {

HighScores tableWith(std::initializer_list<int> scores) {
    HighScores t;
    int i = 0;
    for (int s : scores) {
        t.add(ScoreEntry{"P" + std::to_string(i++), s, 1});
    }
    return t;
}

HighScores fullTable() {
    return tableWith({1000, 900, 800, 700, 600, 500, 400, 300, 200, 100});
}

} // namespace

TEST(HighScoresTest, EmptyByDefault) {
    HighScores t;
    EXPECT_TRUE(t.empty());
}

TEST(HighScoresTest, AnyPositiveScoreQualifiesForEmptyTable) {
    HighScores t;
    EXPECT_TRUE(t.qualifies(10));
    EXPECT_FALSE(t.qualifies(0));
    EXPECT_FALSE(t.qualifies(-5));
}

TEST(HighScoresTest, KeptSortedByScore) {
    HighScores t = tableWith({300, 1000, 500});
    ASSERT_EQ(t.entries().size(), 3u);
    EXPECT_EQ(t.entries()[0].score, 1000);
    EXPECT_EQ(t.entries()[1].score, 500);
    EXPECT_EQ(t.entries()[2].score, 300);
}

TEST(HighScoresTest, AddReturnsPlace) {
    HighScores t = tableWith({1000, 500});
    EXPECT_EQ(t.add(ScoreEntry{"NEW", 700, 2}), 1);
    EXPECT_EQ(t.entries()[1].name, "NEW");
}

TEST(HighScoresTest, NewEqualScoreGoesBelowOld) {
    HighScores t = tableWith({500});
    EXPECT_EQ(t.add(ScoreEntry{"NEW", 500, 1}), 1);
    EXPECT_EQ(t.entries()[0].name, "P0");
}

TEST(HighScoresTest, KeepsOnlyTopTen) {
    HighScores t = fullTable();
    EXPECT_EQ(t.add(ScoreEntry{"NEW", 650, 1}), 4);
    ASSERT_EQ(t.entries().size(), HighScores::kMaxEntries);
    EXPECT_EQ(t.entries().back().score, 200);  // 100 вылетел
}

TEST(HighScoresTest, LowScoreDoesNotGetIntoFullTable) {
    HighScores t = fullTable();
    EXPECT_FALSE(t.qualifies(100));  // нужно строго больше последнего
    EXPECT_EQ(t.rankFor(50), -1);
    EXPECT_EQ(t.add(ScoreEntry{"LOW", 50, 1}), -1);
    EXPECT_EQ(t.entries().back().score, 100);
}

TEST(HighScoresTest, RankForMatchesAdd) {
    HighScores t = fullTable();
    const int predicted = t.rankFor(850);
    EXPECT_EQ(t.add(ScoreEntry{"X", 850, 1}), predicted);
}

TEST(HighScoresTest, WriteThenReadGivesSameTable) {
    HighScores t = tableWith({1200, 300});
    t.add(ScoreEntry{"Тимур", 800, 5});
    std::stringstream buffer;
    t.write(buffer);

    HighScores loaded;
    loaded.read(buffer);
    ASSERT_EQ(loaded.entries().size(), 3u);
    EXPECT_EQ(loaded.entries()[1].name, "Тимур");
    EXPECT_EQ(loaded.entries()[1].score, 800);
    EXPECT_EQ(loaded.entries()[1].runs, 5);
}

TEST(HighScoresTest, ReadSkipsBrokenLinesAndComments) {
    std::istringstream in(
        "# комментарий\n"
        "500;2;ANNA\r\n"        // файл с Windows
        "мусор\n"
        "abc;1;BAD\n"
        "0;1;ZERO\n"            // ноль очков не рекорд
        "900;4;BORIS\n"
        "\n");
    HighScores t;
    t.read(in);
    ASSERT_EQ(t.entries().size(), 2u);
    EXPECT_EQ(t.entries()[0].name, "BORIS");  // заодно отсортировано
    EXPECT_EQ(t.entries()[1].name, "ANNA");
}

TEST(HighScoresTest, SaveAndLoadRealFile) {
    const auto path = (std::filesystem::temp_directory_path() / "horace_scores_test.txt").string();
    HighScores t = tableWith({700, 400});
    ASSERT_TRUE(t.save(path));

    HighScores loaded;
    ASSERT_TRUE(loaded.load(path));
    EXPECT_EQ(loaded.entries().size(), 2u);
    EXPECT_EQ(loaded.entries()[0].score, 700);
    std::remove(path.c_str());
}

TEST(HighScoresTest, LoadMissingFileGivesEmptyTable) {
    HighScores t = tableWith({700});
    EXPECT_FALSE(t.load("/no/such/dir/scores.txt"));
    EXPECT_TRUE(t.empty());
}

TEST(HighScoresTest, NameIsCleanedUp) {
    EXPECT_EQ(HighScores::sanitizeName("  MAX  "), "MAX");
    EXPECT_EQ(HighScores::sanitizeName("A;B\nC"), "ABC");       // ';' сломал бы файл
    EXPECT_EQ(HighScores::sanitizeName(""), "ИГРОК");
    EXPECT_EQ(HighScores::sanitizeName("   "), "ИГРОК");
    EXPECT_EQ(HighScores::sanitizeName("ABCDEFGHIJK"), "ABCDEFGH");
    EXPECT_EQ(HighScores::sanitizeName("Александровна"), "Александ");  // 8 букв, а не 8 байт
}

TEST(Utf8Test, LengthCountsLettersNotBytes) {
    EXPECT_EQ(horace::utf8::length("ABC"), 3u);
    EXPECT_EQ(horace::utf8::length("Жук"), 3u);  // 6 байт, 3 буквы
}

TEST(Utf8Test, AppendAndPopRussianLetter) {
    std::string s = "A";
    horace::utf8::append(s, U'Ж');
    EXPECT_EQ(s, "AЖ");
    EXPECT_EQ(s.size(), 3u);
    horace::utf8::popBack(s);
    EXPECT_EQ(s, "A");
    horace::utf8::popBack(s);
    horace::utf8::popBack(s);  // из пустой строки — ничего страшного
    EXPECT_TRUE(s.empty());
}
