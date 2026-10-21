#include "core/HighScores.hpp"

#include <algorithm>
#include <fstream>
#include <istream>
#include <ostream>
#include <sstream>

namespace horace {

// ---------------------------------------------------------------------------
// UTF-8
// ---------------------------------------------------------------------------
namespace utf8 {

namespace {
// Байты-«продолжения» в UTF-8 выглядят как 10xxxxxx
bool isContinuation(unsigned char byte) { return (byte & 0xC0) == 0x80; }
} // namespace

size_t length(const std::string& s) {
    size_t count = 0;
    for (unsigned char byte : s) {
        if (!isContinuation(byte)) ++count;
    }
    return count;
}

void append(std::string& s, char32_t c) {
    if (c < 0x80) {
        s += static_cast<char>(c);
    } else if (c < 0x800) {
        s += static_cast<char>(0xC0 | (c >> 6));
        s += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        s += static_cast<char>(0xE0 | (c >> 12));
        s += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        s += static_cast<char>(0xF0 | (c >> 18));
        s += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        s += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        s += static_cast<char>(0x80 | (c & 0x3F));
    }
}

void popBack(std::string& s) {
    while (!s.empty() && isContinuation(static_cast<unsigned char>(s.back()))) {
        s.pop_back();
    }
    if (!s.empty()) {
        s.pop_back();  // первый байт символа
    }
}

} // namespace utf8

// ---------------------------------------------------------------------------
// HighScores
// ---------------------------------------------------------------------------

bool HighScores::qualifies(int score) const {
    if (score <= 0) {
        return false;
    }
    return entries_.size() < kMaxEntries || score > entries_.back().score;
}

int HighScores::rankFor(int score) const {
    if (!qualifies(score)) {
        return -1;
    }
    // Встаём перед первым, у кого очков строго меньше
    for (size_t i = 0; i < entries_.size(); ++i) {
        if (entries_[i].score < score) {
            return static_cast<int>(i);
        }
    }
    return static_cast<int>(entries_.size());
}

int HighScores::add(ScoreEntry entry) {
    const int rank = rankFor(entry.score);
    if (rank < 0) {
        return -1;
    }
    entry.name = sanitizeName(entry.name);
    entries_.insert(entries_.begin() + rank, std::move(entry));
    if (entries_.size() > kMaxEntries) {
        entries_.pop_back();
    }
    return rank;
}

std::string HighScores::sanitizeName(const std::string& name) {
    std::string clean;
    for (char ch : name) {
        if (ch != ';' && ch != '\n' && ch != '\r' && ch != '\t') {
            clean += ch;
        }
    }
    // пробелы по краям не нужны
    const size_t first = clean.find_first_not_of(' ');
    if (first == std::string::npos) {
        return "ИГРОК";
    }
    clean = clean.substr(first, clean.find_last_not_of(' ') - first + 1);

    while (utf8::length(clean) > kMaxNameLength) {
        utf8::popBack(clean);
    }
    return clean;
}

void HighScores::write(std::ostream& out) const {
    out << "# Horace Goes Skiing: очки;заезды;имя\n";
    for (const ScoreEntry& e : entries_) {
        out << e.score << ';' << e.runs << ';' << e.name << '\n';
    }
}

void HighScores::read(std::istream& in) {
    entries_.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();  // файл мог быть сохранён на Windows
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }
        // очки;заезды;имя — испорченные строки просто пропускаем
        std::istringstream fields(line);
        ScoreEntry e;
        char sep1 = 0;
        char sep2 = 0;
        if (!(fields >> e.score >> sep1 >> e.runs >> sep2) || sep1 != ';' || sep2 != ';') {
            continue;
        }
        std::getline(fields, e.name);
        if (e.score <= 0) {
            continue;
        }
        e.name = sanitizeName(e.name);
        entries_.push_back(std::move(e));
    }
    // Файл могли поправить руками — наводим порядок
    std::stable_sort(entries_.begin(), entries_.end(),
                     [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (entries_.size() > kMaxEntries) {
        entries_.resize(kMaxEntries);
    }
}

bool HighScores::save(const std::string& path) const {
    std::ofstream out(path);
    if (!out) {
        return false;
    }
    write(out);
    return static_cast<bool>(out);
}

bool HighScores::load(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        entries_.clear();
        return false;
    }
    read(in);
    return true;
}

} // namespace horace
