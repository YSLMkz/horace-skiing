#include "app/Renderer.hpp"

#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include <cmath>

namespace horace {

namespace {

const sf::Color kSnow(240, 240, 255);
const sf::Color kAsphalt(60, 60, 60);
const sf::Color kLaneMark(200, 200, 200);
const sf::Color kCar(220, 60, 60);
const sf::Color kShop(150, 90, 40);
const sf::Color kShopDoor(80, 45, 20);
const sf::Color kHorace(90, 200, 90);
const sf::Color kSkis(30, 60, 200);
const sf::Color kText(20, 20, 40);
const sf::Color kFlag(220, 40, 40);
const sf::Color kFlagBlue(40, 80, 220);
const sf::Color kCenterLine(240, 200, 40);

void drawRect(sf::RenderTarget& target, const Rect& r, sf::Color color) {
    sf::RectangleShape shape({r.width, r.height});
    shape.setPosition(r.x, r.y);
    shape.setFillColor(color);
    target.draw(shape);
}

// Рисует текстуру, растянутую на прямоугольник r.
// flipped — отразить по горизонтали (машина едет влево, а нарисована мордой вправо).
// color — «перекрасить»: цвета картинки умножаются на него (белое станет color,
// тёмное останется тёмным). Так один белый флажок становится красным или синим.
void drawSprite(sf::RenderTarget& target, const sf::Texture& texture, const Rect& r, bool flipped,
                sf::Color color = sf::Color::White) {
    sf::Sprite sprite(texture);
    sprite.setColor(color);
    const sf::Vector2u size = texture.getSize();
    const float sx = r.width / static_cast<float>(size.x);
    const float sy = r.height / static_cast<float>(size.y);
    if (flipped) {
        sprite.setScale(-sx, sy);         // отрицательный масштаб — зеркально
        sprite.setPosition(r.x + r.width, r.y);
    } else {
        sprite.setScale(sx, sy);
        sprite.setPosition(r.x, r.y);
    }
    target.draw(sprite);
}

} // namespace

bool Renderer::loadFont(const std::string& path, bool pixelFont) {
    hasFont_ = font_.loadFromFile(path);
    if (hasFont_ && pixelFont) {
        // Пиксельный шрифт нельзя сглаживать — иначе квадратные пиксели
        // размоются в кашу. Размеры текста тогда берём кратными 8.
        font_.setSmooth(false);
    }
    return hasFont_;
}

bool Renderer::loadTextures(const std::string& dir) {
    // Порядок важен: номер файла = номер модели машины (Car::model())
    const char* carFiles[RoadLevel::kCarModels] = {
        "car_red.png", "car_yellow.png", "car_blue.png", "van_green.png"};

    carTextures_.clear();
    bool ok = true;
    for (const char* file : carFiles) {
        sf::Texture texture;
        if (!texture.loadFromFile(dir + "/" + file)) {
            ok = false;
            break;
        }
        carTextures_.push_back(texture);
    }
    if (!ok) {
        carTextures_.clear();  // не хватает хоть одной — рисуем все машины прямоугольниками
    }

    hasAmbulanceTextures_ = ambulanceFrames_[0].loadFromFile(dir + "/ambulance_1.png") &&
                            ambulanceFrames_[1].loadFromFile(dir + "/ambulance_2.png");

    // Необязательные спрайты: что нашлось — то и рисуем
    sprites_.clear();
    for (const char* name : {"hero_walk_1", "hero_walk_2", "hero_skis_1", "hero_skis_2", "hero_down",
                             "skier_straight", "skier_turn", "skier_crash",
                             "tree", "tree_hit", "flag", "shop"}) {
        sf::Texture texture;
        if (texture.loadFromFile(dir + "/" + name + ".png")) {
            sprites_[name] = texture;
        }
    }
    return ok && hasAmbulanceTextures_;
}

const sf::Texture* Renderer::sprite(const std::string& name) const {
    const auto it = sprites_.find(name);
    return it == sprites_.end() ? nullptr : &it->second;
}

void Renderer::draw(sf::RenderTarget& target, const Game& game) const {
    target.clear(kSnow);
    const float w = RoadLevel::kWidth;
    const float h = RoadLevel::kHeight;

    switch (game.state()) {
    case GameState::Menu:
        drawRoad(target, game.road(), false);  // на фоне едут машины
        drawMenu(target, game);
        if (game.isDemoMode()) {
            drawText(target, "ДЕМО-РЕЖИМ", 8, 8, 16, false, kFlag);
        }
        return;                                // без HUD
    case GameState::Road:
        drawRoad(target, game.road());
        break;
    case GameState::Slope:
        drawSlope(target, game.slope());
        break;
    case GameState::RunResults:
        drawSlope(target, game.slope());  // склон «замер» на фоне
        drawRunResults(target, game);
        break;
    case GameState::EnterName:
        drawRoad(target, game.road());
        drawEnterName(target, game);
        break;
    case GameState::GameOver:
        drawRoad(target, game.road());
        drawGameOver(target, game);
        break;
    }
    drawHud(target, game);
    if (game.isDemoMode()) {
        // под деньгами и очками, красным
        drawText(target, "ДЕМО-РЕЖИМ", 8, 80, 16, false, kFlag);
    }

    if (game.isPaused()) {
        drawRect(target, Rect{0, h / 2 - 50, w, 100}, sf::Color(255, 255, 255, 220));
        drawText(target, "ПАУЗА", w / 2, h / 2 - 40, 40, true);
        drawText(target, "P - продолжить", w / 2, h / 2 + 16, 16, true);
    }
}

void Renderer::drawPanel(sf::RenderTarget& target, const Rect& r) const {
    sf::RectangleShape panel({r.width, r.height});
    panel.setPosition(r.x, r.y);
    panel.setFillColor(sf::Color(250, 250, 255, 248));
    panel.setOutlineColor(kText);
    panel.setOutlineThickness(3.f);
    target.draw(panel);
}

void Renderer::drawBlinkingText(sf::RenderTarget& target, const std::string& utf8, float x, float y,
                                unsigned size) const {
    // 0.6 с видно, 0.3 с нет
    if (std::fmod(clock_.getElapsedTime().asSeconds(), 0.9f) < 0.6f) {
        drawText(target, utf8, x, y, size, true);
    }
}

void Renderer::drawMenu(sf::RenderTarget& target, const Game& game) const {
    // Размеры текста кратны 8 (размер «пикселя» шрифта Press Start 2P):
    // 16 — обычный текст, 24 — крупный, 32 — заголовок.
    // Шрифт широкий: строка при размере 16 вмещает ~40 символов на 640 px.
    const float cx = RoadLevel::kWidth / 2;
    drawPanel(target, Rect{56, 56, 688, 488});

    drawText(target, "HORACE GOES SKIING", cx, 80, 32, true);
    drawText(target, "аналог игры для ZX Spectrum, 1982", cx, 128, 16, true);

    // Как в «демо-режиме» игровых автоматов: правила и таблица рекордов
    // сменяют друг друга каждые 7 секунд
    const bool showScores = !game.highScores().empty() &&
                            static_cast<int>(clock_.getElapsedTime().asSeconds() / 7.f) % 2 == 1;
    if (showScores) {
        drawText(target, "ЛУЧШИЕ ИГРОКИ", cx, 176, 24, true);
        drawHighScoreTable(target, game.highScores(), 216, -1);
    } else {
        float y = 176;
        for (const char* line : {
                 "Перейди дорогу, возьми лыжи,",
                 "вернись и съезжай меж флажков.",
                 "",
                 "Старт $40. Лыжи $10. Скорая $10.",
                 "Ворота +10, пропуск -10,",
                 "финиш +100. 1000 очков = +$10.",
                 "",
                 "Дорога: шаг стрелками",
                 "Склон:  ← → поворот, ↓ быстрее",
                 "P пауза  M музыка  N звуки",
             }) {
            drawText(target, line, 88, y, 16);
            y += 26;
        }
    }
    drawBlinkingText(target, "НАЖМИТЕ ENTER", cx, 472, 24);
}

void Renderer::drawHighScoreTable(sf::RenderTarget& target, const HighScores& table, float y,
                                  int highlight) const {
    const auto& entries = table.entries();
    if (entries.empty()) {
        drawText(target, "Рекордов пока нет", RoadLevel::kWidth / 2, y + 40, 16, true);
        return;
    }
    // Шрифт моноширинный, поэтому столбцы выравниваем пробелами:
    // " 1. ИМЯ.....   1234"
    // ширина строки: 3 + 1 + 8 + 7 = 19 символов по 16 px — ставим по центру
    const float rowWidth = 19 * 16.f;
    const float left = std::round((RoadLevel::kWidth - rowWidth) / 2);
    for (size_t i = 0; i < entries.size(); ++i) {
        const ScoreEntry& e = entries[i];
        std::string rank = std::to_string(i + 1) + ".";
        if (rank.size() < 3) rank = " " + rank;
        std::string name = e.name;
        for (size_t n = utf8::length(name); n < HighScores::kMaxNameLength; ++n) name += ' ';
        std::string score = std::to_string(e.score);
        while (score.size() < 7) score = " " + score;

        if (static_cast<int>(i) == highlight) {
            drawRect(target, Rect{left - 8, y - 4, rowWidth + 16, 24}, sf::Color(255, 220, 90));
        }
        drawText(target, rank + " " + name + score, left, y, 16);
        y += 24;
    }
}

void Renderer::drawEnterName(sf::RenderTarget& target, const Game& game) const {
    const float cx = RoadLevel::kWidth / 2;
    drawPanel(target, Rect{88, 104, 624, 392});
    drawText(target, "НОВЫЙ РЕКОРД!", cx, 136, 32, true);
    drawText(target, "Очки: " + std::to_string(game.score()), cx, 200, 24, true);
    drawText(target, "Место в таблице: " + std::to_string(game.highScores().rankFor(game.score()) + 1),
             cx, 240, 16, true);

    drawText(target, "Введите имя:", cx, 296, 16, true);
    drawRect(target, Rect{cx - 152, 328, 304, 40}, kText);
    drawRect(target, Rect{cx - 148, 332, 296, 32}, sf::Color(250, 250, 255));
    // мигающий курсор «_» в конце имени
    const bool cursorOn = std::fmod(clock_.getElapsedTime().asSeconds(), 0.8f) < 0.5f;
    const bool full = utf8::length(game.nameInput()) >= HighScores::kMaxNameLength;
    drawText(target, game.nameInput() + (cursorOn && !full ? "_" : ""), cx - 136, 340, 16);

    drawText(target, "буквы и цифры, до 8 символов", cx, 392, 16, true);
    drawText(target, "ENTER - СОХРАНИТЬ", cx, 440, 24, true);
}

void Renderer::drawRunResults(sf::RenderTarget& target, const Game& game) const {
    const RunStats& r = game.lastRun();
    const float cx = RoadLevel::kWidth / 2;
    drawPanel(target, Rect{112, 80, 576, 440});

    const std::string title = "ЗАЕЗД " + std::to_string(r.number) +
                              (r.finished ? ": ФИНИШ!" : ": ЛЫЖИ СЛОМАНЫ");
    drawText(target, title, cx, 104, 24, true);

    auto line = [&](float y, const std::string& label, const std::string& value) {
        drawText(target, label, 144, y, 16);
        drawText(target, value, 472, y, 16);
    };
    float y = 168;
    line(y, "Ворота пройдены", std::to_string(r.gatesPassed) + " (+" + std::to_string(Game::kGatePoints) + ")"); y += 32;
    line(y, "Ворота пропущены", std::to_string(r.gatesMissed) + " (-" + std::to_string(Game::kGatePenalty) + ")"); y += 32;
    line(y, "Деревья", std::to_string(r.treesHit)); y += 32;
    line(y, "Бонус за финиш", r.finished ? "+" + std::to_string(Game::kFinishBonus) : "нет"); y += 48;
    line(y, "Очки за заезд", (r.points >= 0 ? "+" : "") + std::to_string(r.points)); y += 32;
    line(y, "Всего очков", std::to_string(game.score())); y += 32;
    if (r.cashBonus > 0) {
        line(y, "Бонус деньгами", "+$" + std::to_string(r.cashBonus));
    }
    drawText(target, r.skisBroken ? "Лыжи сломаны: прокат $10" : "Лыжи целы, сразу к склону",
             cx, 432, 16, true);
    drawBlinkingText(target, "ENTER - ДАЛЬШЕ", cx, 472, 24);
}

void Renderer::drawGameOver(sf::RenderTarget& target, const Game& game) const {
    const float cx = RoadLevel::kWidth / 2;
    drawPanel(target, Rect{56, 56, 688, 488});
    drawText(target, "ДЕНЬГИ КОНЧИЛИСЬ", cx, 80, 24, true);
    drawText(target, "Очки " + std::to_string(game.score()) + "   Заездов " +
                         std::to_string(game.runsCompleted()),
             cx, 120, 16, true);
    if (game.lastRank() >= 0) {
        drawText(target, "Новый рекорд! Место " + std::to_string(game.lastRank() + 1),
                 cx, 148, 16, true);
    }
    drawText(target, "ЛУЧШИЕ ИГРОКИ", cx, 184, 16, true);
    drawHighScoreTable(target, game.highScores(), 216, game.lastRank());
    drawBlinkingText(target, "ENTER - В МЕНЮ", cx, 480, 24);
}

void Renderer::drawRoad(sf::RenderTarget& target, const RoadLevel& road, bool withPlayer) const {
    const float cell = RoadLevel::kCell;

    // асфальт (строки 4..10): пунктир между полосами одного направления,
    // двойная сплошная посередине (строка 7) — между встречными
    drawRect(target, Rect{0, 4 * cell, RoadLevel::kWidth, 7 * cell}, kAsphalt);
    for (float row : {5.f, 9.f}) {
        for (float x = 0; x < RoadLevel::kWidth; x += 2 * cell) {
            drawRect(target, Rect{x, row * cell + cell / 2 - 2, cell, 4}, kLaneMark);
        }
    }
    drawRect(target, Rect{0, 7 * cell + cell / 2 - 6, RoadLevel::kWidth, 3}, kCenterLine);
    drawRect(target, Rect{0, 7 * cell + cell / 2 + 3, RoadLevel::kWidth, 3}, kCenterLine);

    // лавка проката с дверью
    const Rect& shop = road.shop();
    if (const sf::Texture* t = sprite("shop")) {
        drawSprite(target, *t, shop, false);
    } else {
        drawRect(target, shop, kShop);
        drawRect(target, Rect{shop.x + shop.width / 2 - 10, shop.y + 10, 20, shop.height - 10}, kShopDoor);
    }
    drawText(target, "ПРОКАТ $10", shop.x + shop.width + 12, shop.y + shop.height / 2 - 8, 16);

    for (const Car& car : road.cars()) {
        drawCar(target, car);
    }

    if (!withPlayer) {
        return;
    }
    drawHorace(target, road);
    if (road.ambulance().isActive()) {
        drawAmbulance(target, road.ambulance());
    }

    // Подсказки короткие: герой стоит внизу по центру, текст не должен до него доставать
    const char* hint = road.hasSkis() ? "Лыжи есть! Вниз!" : "Иди к лавке за лыжами";
    if (road.ambulance().isActive()) {
        hint = "Ой! Едет скорая, это будет стоить $10";
    }
    drawText(target, hint, 8, RoadLevel::kHeight - 28, 16);
}

void Renderer::drawCar(sf::RenderTarget& target, const Car& car) const {
    if (carTextures_.empty()) {
        drawRect(target, car.bounds(), kCar);
        return;
    }
    const sf::Texture& texture = carTextures_[static_cast<size_t>(car.model()) % carTextures_.size()];
    drawSprite(target, texture, car.bounds(), car.speed() < 0.f);
}

void Renderer::drawHorace(sf::RenderTarget& target, const RoadLevel& road) const {
    if (road.isHoraceInAmbulance()) {
        return;  // уже внутри скорой
    }
    const Rect& hb = road.horace().bounds();

    // Картинка героя крупнее его клетки (48x48 вместо 40x40): немного выступает
    // вверх и в стороны, а столкновения по-прежнему считаются по клетке 40x40.
    // Так герой лучше смотрится рядом с машинами, а сбивают его по-честному.
    const Rect look{hb.x - 4, hb.y - 8, hb.width + 8, hb.height + 8};

    if (road.isHoraceKnockedDown()) {
        if (const sf::Texture* t = sprite("hero_down")) {
            drawSprite(target, *t, look, false);
        } else {
            // лежит на дороге: плоский прямоугольник внизу клетки
            drawRect(target, Rect{hb.x - 4, hb.y + hb.height - 14, hb.width + 8, 14}, kHorace);
            if (road.hasSkis()) {
                drawRect(target, Rect{hb.x - 10, hb.y + hb.height - 4, hb.width + 20, 4}, kSkis);
            }
        }
        // «звёздочки» над головой мигают
        const float t = road.ambulance().time();
        const sf::Color star(255, 220, 40);
        for (int i = 0; i < 3; ++i) {
            const float phase = t * 6.f + static_cast<float>(i) * 2.1f;
            if (std::sin(phase) > 0.f) {
                drawRect(target, Rect{hb.x + 4 + i * 12.f, hb.y + 8 + (i % 2) * 6.f, 5, 5}, star);
            }
        }
        return;
    }

    // Анимация шага: Хорас ходит по клеткам, и с каждым шагом сумма его клеток
    // по x и y меняет чётность — значит, кадры 1 и 2 чередуются сами собой
    const int frame = static_cast<int>((hb.x + hb.y) / RoadLevel::kCell) % 2;
    const char* name = road.hasSkis() ? (frame ? "hero_skis_2" : "hero_skis_1")
                                      : (frame ? "hero_walk_2" : "hero_walk_1");
    if (const sf::Texture* t = sprite(name)) {
        drawSprite(target, *t, look, false);
        return;
    }
    drawRect(target, hb, kHorace);
    if (road.hasSkis()) {
        // лыжи на плече — две синие полоски
        drawRect(target, Rect{hb.x - 6, hb.y + 8, hb.width + 12, 5}, kSkis);
        drawRect(target, Rect{hb.x - 6, hb.y + 16, hb.width + 12, 5}, kSkis);
    }
}

void Renderer::drawAmbulance(sf::RenderTarget& target, const Ambulance& ambulance) const {
    const Rect& r = ambulance.bounds();
    // Мигалка: каждые 0.15 секунды меняем кадр — это и есть анимация
    const int frame = static_cast<int>(ambulance.time() / 0.15f) % 2;

    if (hasAmbulanceTextures_) {
        drawSprite(target, ambulanceFrames_[frame], r, false);
        return;
    }
    // запасной вариант без текстур
    drawRect(target, r, sf::Color(235, 235, 235));
    drawRect(target, Rect{r.x + 28, r.y + 8, 8, 24}, kFlag);
    drawRect(target, Rect{r.x + 20, r.y + 16, 24, 8}, kFlag);
    drawRect(target, Rect{r.x + 36, r.y - 4, 10, 4}, frame ? sf::Color(25, 40, 90) : sf::Color(80, 160, 255));
}

void Renderer::drawSlope(sf::RenderTarget& target, const SlopeLevel& slope) const {
    // Всё на склоне хранится в мировых координатах.
    // Чтобы нарисовать на экране, вычитаем положение камеры по y.
    const float cam = slope.cameraY();
    auto toScreen = [cam](Rect r) { r.y -= cam; return r; };
    auto visible = [cam](float worldY) { return worldY > cam - 60 && worldY < cam + SlopeLevel::kScreenHeight + 60; };

    // финишная линия — чёрно-белые клетки
    if (visible(slope.courseLength())) {
        const float y = slope.courseLength() - cam;
        for (int i = 0; i * 20 < SlopeLevel::kWidth; ++i) {
            drawRect(target, Rect{i * 20.f, y, 20, 10}, i % 2 ? sf::Color::Black : sf::Color::White);
            drawRect(target, Rect{i * 20.f, y + 10, 20, 10}, i % 2 ? sf::Color::White : sf::Color::Black);
        }
        drawText(target, "ФИНИШ", SlopeLevel::kWidth / 2, y + 28, 32, true);
    }

    // ворота: два флажка, как в оригинале — пары по очереди красные и синие;
    // пропущенные ворота сереют
    const auto& gates = slope.gates();
    for (size_t i = 0; i < gates.size(); ++i) {
        const Gate& gate = gates[i];
        if (!visible(gate.y)) continue;
        sf::Color flag = i % 2 == 0 ? kFlag : kFlagBlue;
        if (gate.resolved && !gate.passed) flag = sf::Color(160, 160, 160);
        if (const sf::Texture* t = sprite("flag")) {
            // белый флажок 8x16, увеличенный вдвое; древко — на краю ворот
            for (float x : {gate.leftX - 3, gate.rightX - 3}) {
                drawSprite(target, *t, toScreen(Rect{x, gate.y - 32, 16, 32}), false, flag);
            }
            continue;
        }
        for (float x : {gate.leftX - 4, gate.rightX - 4}) {
            drawRect(target, toScreen(Rect{x + 3, gate.y - 30, 2, 30}), sf::Color(60, 60, 60));  // древко
            drawRect(target, toScreen(Rect{x + 5, gate.y - 30, 14, 10}), flag);                 // флажок
        }
    }

    // деревья: зелёный треугольник + ствол
    for (const Tree& tree : slope.trees()) {
        if (!visible(tree.bounds.y)) continue;
        const Rect r = toScreen(tree.bounds);
        const sf::Texture* t = sprite(tree.hit ? "tree_hit" : "tree");
        if (!t) t = sprite("tree");  // задетой ёлки может не быть — тогда обычная
        if (t) {
            drawSprite(target, *t, r, false);
            continue;
        }
        sf::ConvexShape crown(3);
        crown.setPoint(0, {r.x + r.width / 2, r.y});
        crown.setPoint(1, {r.x, r.y + r.height - 8});
        crown.setPoint(2, {r.x + r.width, r.y + r.height - 8});
        crown.setFillColor(tree.hit ? sf::Color(120, 140, 120) : sf::Color(30, 120, 50));
        target.draw(crown);
        drawRect(target, Rect{r.x + r.width / 2 - 3, r.y + r.height - 8, 6, 8}, kShopDoor);
    }

    // лыжник: упал / поворачивает / едет прямо. Поворот нарисован вправо,
    // влево — тот же спрайт, отражённый зеркально.
    const Rect s = toScreen(slope.skier());
    const char* skierName = slope.isCrashed()     ? "skier_crash"
                            : slope.steer() != 0  ? "skier_turn"
                                                  : "skier_straight";
    const sf::Texture* skierTexture = sprite(skierName);
    if (!skierTexture) skierTexture = sprite("skier_straight");
    if (skierTexture) {
        drawSprite(target, *skierTexture, s, !slope.isCrashed() && slope.steer() < 0);
    } else {
        // запасной вариант: тело + две лыжи вниз; после удара о дерево — серый
        drawRect(target, Rect{s.x + 3, s.y, s.width - 6, s.height - 6},
                 slope.isCrashed() ? sf::Color(150, 150, 150) : kHorace);
        drawRect(target, Rect{s.x + 2, s.y + 4, 4, s.height}, kSkis);
        drawRect(target, Rect{s.x + s.width - 6, s.y + 4, 4, s.height}, kSkis);
    }

    // состояние лыж: «Лыжи:» и квадратики — целые залиты, сломанные пустые
    drawText(target, "Лыжи:", SlopeLevel::kWidth - 176, 12, 16);
    for (int i = 0; i < SlopeLevel::kMaxDamage; ++i) {
        const Rect box{SlopeLevel::kWidth - 88 + i * 24.f, 10, 16, 16};
        drawRect(target, box, kText);
        if (i >= SlopeLevel::kMaxDamage - slope.damage()) {
            drawRect(target, Rect{box.x + 3, box.y + 3, box.width - 6, box.height - 6}, kSnow);
        }
    }
    drawText(target, "← → поворот, ↓ быстрее", 8, SlopeLevel::kScreenHeight - 28, 16);
}

void Renderer::drawHud(sf::RenderTarget& target, const Game& game) const {
    drawText(target, "$" + std::to_string(game.money()), 8, 8, 24);
    drawText(target, "Очки " + std::to_string(game.score()), 8, 40, 16);
    drawText(target, "+$10 при " + std::to_string(game.nextCashAt()), 8, 60, 8);
}

void Renderer::drawText(sf::RenderTarget& target, const std::string& utf8, float x, float y,
                        unsigned size, bool centered, sf::Color color) const {
    if (!hasFont_) {
        return;
    }
    // Исходники в UTF-8, поэтому русский текст переводим через fromUtf8
    sf::Text text(sf::String::fromUtf8(utf8.begin(), utf8.end()), font_, size);
    text.setFillColor(color);
    if (centered) {
        const sf::FloatRect b = text.getLocalBounds();
        text.setOrigin(std::round(b.left + b.width / 2.f), 0.f);
    }
    // Целые координаты: при дробных пиксельный шрифт рисуется неровно
    text.setPosition(std::round(x), std::round(y));
    target.draw(text);
}

} // namespace horace
