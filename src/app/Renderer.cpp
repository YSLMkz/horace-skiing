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

bool Renderer::loadFont(const std::string& path) {
    hasFont_ = font_.loadFromFile(path);
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
    case GameState::Road:
        drawRoad(target, game.road());
        break;
    case GameState::Slope:
        drawSlope(target, game.slope());
        break;
    case GameState::GameOver:
        drawRoad(target, game.road());
        drawRect(target, Rect{0, h / 2 - 70, w, 140}, sf::Color(255, 255, 255, 220));
        drawText(target, "ДЕНЬГИ КОНЧИЛИСЬ", w / 2, h / 2 - 50, 40, true);
        drawText(target, "R — начать заново, Esc — выход", w / 2, h / 2 + 10, 20, true);
        break;
    }
    drawHud(target, game);
}

void Renderer::drawRoad(sf::RenderTarget& target, const RoadLevel& road) const {
    const float cell = RoadLevel::kCell;

    // асфальт (строки 4..10) и пунктир между полосами
    drawRect(target, Rect{0, 4 * cell, RoadLevel::kWidth, 7 * cell}, kAsphalt);
    for (float row : {5.f, 7.f, 9.f}) {
        for (float x = 0; x < RoadLevel::kWidth; x += 2 * cell) {
            drawRect(target, Rect{x, row * cell + cell / 2 - 2, cell, 4}, kLaneMark);
        }
    }

    // лавка проката с дверью
    const Rect& shop = road.shop();
    drawRect(target, shop, kShop);
    drawRect(target, Rect{shop.x + shop.width / 2 - 10, shop.y + 10, 20, shop.height - 10}, kShopDoor);
    drawText(target, "ПРОКАТ $10", shop.x + shop.width + 10, shop.y + 8, 18);

    for (const Car& car : road.cars()) {
        drawCar(target, car);
    }

    drawHorace(target, road);
    if (road.ambulance().isActive()) {
        drawAmbulance(target, road.ambulance());
    }

    const char* hint = road.hasSkis() ? "Лыжи есть! Вниз, к склону" : "Дойди до лавки за лыжами";
    if (road.ambulance().isActive()) {
        hint = "Ой! Едет скорая — это будет стоить $10";
    }
    drawText(target, hint, 10, RoadLevel::kHeight - 30, 18);
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
        drawText(target, "ФИНИШ", SlopeLevel::kWidth / 2, y + 26, 28, true);
    }

    // ворота: два флажка; пройденные — зелёные, пропущенные — серые
    for (const Gate& gate : slope.gates()) {
        if (!visible(gate.y)) continue;
        sf::Color flag = kFlag;
        if (gate.resolved) flag = gate.passed ? kHorace : sf::Color(160, 160, 160);
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

    // состояние лыж
    std::string skis = "Лыжи: ";
    for (int i = 0; i < SlopeLevel::kMaxDamage; ++i) {
        skis += i < SlopeLevel::kMaxDamage - slope.damage() ? "■" : "□";
    }
    drawText(target, skis, SlopeLevel::kWidth - 170, 8, 20);
    drawText(target, "← → поворот, ↓ быстрее", 10, SlopeLevel::kScreenHeight - 30, 18);
}

void Renderer::drawHud(sf::RenderTarget& target, const Game& game) const {
    drawText(target, "$" + std::to_string(game.money()), 10, 8, 22);
    drawText(target, "Очки: " + std::to_string(game.score()), 10, 36, 18);
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
        text.setOrigin(b.left + b.width / 2.f, 0.f);
    }
    text.setPosition(x, y);
    target.draw(text);
}

} // namespace horace
