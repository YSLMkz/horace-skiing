#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace horace {

// Одна строка таблицы рекордов
struct ScoreEntry {
    std::string name;  // в UTF-8 (можно по-русски)
    int score = 0;
    int runs = 0;      // сколько заездов было в той игре
};

// Таблица рекордов: лучшие kMaxEntries результатов по убыванию очков.
//
// Хранится в текстовом файле, по строке на запись:
//     очки;заезды;имя
// Строки, которые начинаются с '#', и испорченные строки при чтении пропускаются.
// Чтение и запись сделаны через потоки (std::istream / std::ostream), поэтому
// их легко проверить в тестах без настоящих файлов.
class HighScores {
public:
    static constexpr size_t kMaxEntries = 10;
    static constexpr size_t kMaxNameLength = 8;  // в символах, не в байтах

    const std::vector<ScoreEntry>& entries() const { return entries_; }
    bool empty() const { return entries_.empty(); }

    // Попадёт ли такой результат в таблицу
    bool qualifies(int score) const;
    // Какое место (с 0) займёт такой результат; -1 — не попадёт
    int rankFor(int score) const;

    // Добавить результат. Возвращает место (с 0) или -1, если не попал.
    // При равенстве очков новый результат встаёт ниже старых.
    int add(ScoreEntry entry);

    void clear() { entries_.clear(); }

    // Чтение и запись
    void write(std::ostream& out) const;
    void read(std::istream& in);  // заменяет текущую таблицу
    bool save(const std::string& path) const;
    bool load(const std::string& path);  // false — файла нет (таблица пустая)

    // Убрать из имени ';' и переводы строк, лишние пробелы, обрезать до
    // kMaxNameLength символов. Пустое имя превращается в «ИГРОК».
    static std::string sanitizeName(const std::string& name);

private:
    std::vector<ScoreEntry> entries_;
};

// Небольшие помощники для UTF-8: в std::string русская буква занимает 2 байта,
// поэтому «длина в символах» и «стереть последний символ» нужно считать отдельно.
namespace utf8 {
size_t length(const std::string& s);              // сколько символов
void append(std::string& s, char32_t codepoint);  // дописать символ
void popBack(std::string& s);                     // стереть последний символ
} // namespace utf8

} // namespace horace
