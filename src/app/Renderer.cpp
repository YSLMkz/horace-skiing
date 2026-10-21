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
void drawSprite(sf::RenderTarget& target, const sf::Texture& texture, const Rect& r, bool flipped) {
    sf::Sprite sprite(texture);
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
    return ok && hasAmbulanceTextures_;
}

void Renderer::draw(sf::RenderTarget& target, const Game& game) const {
    target.clear(kSnow);
    const float w = RoadLevel::kWidth;
    const float h = RoadLevel::kHeight;

    switch (game.state()) {
    case GameState::Menu:
        drawRoad(target, game.road(), false);  // на фоне едут машины
        drawMenu(target);
        return;                         // без HUD
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
    case GameState::GameOver:
        drawRoad(target, game.road());
        drawGameOver(target, game);
        break;
    }
    drawHud(target, game);

    if (game.isPaused()) {
        drawRect(target, Rect{0, h / 2 - 50, w, 100}, sf::Color(255, 255, 255, 220));
        drawText(target, "ПАУЗА", w / 2, h / 2 - 40, 40, true);
        drawText(target, "P — продолжить", w / 2, h / 2 + 16, 16, true);
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

void Renderer::drawMenu(sf::RenderTarget& target) const {
    // Размеры текста кратны 8 (размер «пикселя» шрифта Press Start 2P):
    // 16 — обычный текст, 24 — крупный, 32 — заголовок.
    // Шрифт широкий: строка при размере 16 вмещает ~40 символов на 640 px.
    const float cx = RoadLevel::kWidth / 2;
    drawPanel(target, Rect{56, 56, 688, 488});

    drawText(target, "HORACE GOES SKIING", cx, 80, 32, true);
    drawText(target, "аналог игры для ZX Spectrum · 1982", cx, 128, 16, true);

    float y = 176;
    for (const char* line : {
             "Перейди дорогу, возьми лыжи,",
             "вернись и съезжай меж флажков.",
             "",
             "Старт $40. Лыжи $10. Скорая $10.",
             "Ворота +10, пропуск -10,",
             "финиш +100. 1000 очков = +$10.",
             "",
             "Дорога: стрелки — шаг",
             "Склон:  ← → поворот, ↓ быстрее",
             "P пауза  M музыка  N звуки",
         }) {
        drawText(target, line, 88, y, 16);
        y += 26;
    }
    drawBlinkingText(target, "НАЖМИТЕ ENTER", cx, 472, 24);
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
    line(y, "Бонус за финиш", r.finished ? "+" + std::to_string(Game::kFinishBonus) : "—"); y += 48;
    line(y, "Очки за заезд", (r.points >= 0 ? "+" : "") + std::to_string(r.points)); y += 32;
    line(y, "Всего очков", std::to_string(game.score())); y += 32;
    if (r.cashBonus > 0) {
        line(y, "Бонус деньгами", "+$" + std::to_string(r.cashBonus));
    }
    drawText(target, r.skisBroken ? "Лыжи сломаны: прокат $10" : "Лыжи целы — сразу к склону",
             cx, 432, 16, true);
    drawBlinkingText(target, "ENTER — ДАЛЬШЕ", cx, 472, 24);
}

void Renderer::drawGameOver(sf::RenderTarget& target, const Game& game) const {
    const float cx = RoadLevel::kWidth / 2;
    drawPanel(target, Rect{160, 152, 480, 296});
    drawText(target, "ДЕНЬГИ КОНЧИЛИСЬ", cx, 184, 24, true);
    drawText(target, "Очки:    " + std::to_string(game.score()), 224, 256, 24);
    drawText(target, "Заездов: " + std::to_string(game.runsCompleted()), 224, 296, 24);
    drawBlinkingText(target, "ENTER — В МЕНЮ", cx, 384, 24);
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
    drawRect(target, shop, kShop);
    drawRect(target, Rect{shop.x + shop.width / 2 - 10, shop.y + 10, 20, shop.height - 10}, kShopDoor);
    drawText(target, "ПРОКАТ $10", shop.x + shop.width + 12, shop.y + 12, 16);

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

    const char* hint = road.hasSkis() ? "Лыжи есть! Вниз, к склону" : "Дойди до лавки за лыжами";
    if (road.ambulance().isActive()) {
        hint = "Ой! Едет скорая — это будет стоить $10";
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

    if (road.isHoraceKnockedDown()) {
        // лежит на дороге: плоский прямоугольник внизу клетки
        drawRect(target, Rect{hb.x - 4, hb.y + hb.height - 14, hb.width + 8, 14}, kHorace);
        if (road.hasSkis()) {
            drawRect(target, Rect{hb.x - 10, hb.y + hb.height - 4, hb.width + 20, 4}, kSkis);
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
        for (float x : {gate.leftX - 4, gate.rightX - 4}) {
            drawRect(target, toScreen(Rect{x + 3, gate.y - 30, 2, 30}), sf::Color(60, 60, 60));  // древко
            drawRect(target, toScreen(Rect{x + 5, gate.y - 30, 14, 10}), flag);                 // флажок
        }
    }

    // деревья: зелёный треугольник + ствол
    for (const Tree& tree : slope.trees()) {
        if (!visible(tree.bounds.y)) continue;
        const Rect r = toScreen(tree.bounds);
        sf::ConvexShape crown(3);
        crown.setPoint(0, {r.x + r.width / 2, r.y});
        crown.setPoint(1, {r.x, r.y + r.height - 8});
        crown.setPoint(2, {r.x + r.width, r.y + r.height - 8});
        crown.setFillColor(tree.hit ? sf::Color(120, 140, 120) : sf::Color(30, 120, 50));
        target.draw(crown);
        drawRect(target, Rect{r.x + r.width / 2 - 3, r.y + r.height - 8, 6, 8}, kShopDoor);
    }

    // лыжник: тело + две лыжи вниз; после удара о дерево — серый
    const Rect s = toScreen(slope.skier());
    drawRect(target, Rect{s.x + 3, s.y, s.width - 6, s.height - 6}, slope.isCrashed() ? sf::Color(150, 150, 150) : kHorace);
    drawRect(target, Rect{s.x + 2, s.y + 4, 4, s.height}, kSkis);
    drawRect(target, Rect{s.x + s.width - 6, s.y + 4, 4, s.height}, kSkis);

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
                        unsigned size, bool centered) const {
    if (!hasFont_) {
        return;
    }
    // Исходники в UTF-8, поэтому русский текст переводим через fromUtf8
    sf::Text text(sf::String::fromUtf8(utf8.begin(), utf8.end()), font_, size);
    text.setFillColor(kText);
    if (centered) {
        const sf::FloatRect b = text.getLocalBounds();
        text.setOrigin(std::round(b.left + b.width / 2.f), 0.f);
    }
    // Целые координаты: при дробных пиксельный шрифт рисуется неровно
    text.setPosition(std::round(x), std::round(y));
    target.draw(text);
}

} // namespace horace
