#include "ui/BattleHud.hpp"
#include "ui/BattleIcons.hpp"
#include "ui/CardView.hpp"
#include "ui/CardPresentation.hpp"
#include "ui/UiHelpers.hpp"

#include <algorithm>
#include <array>

namespace
{
using Kind = BattleIcons::Kind;
const sf::Color paper(246,235,208), muted(171,178,185), accent(235,189,102);
constexpr int pageSize = 10;
const sf::FloatRect panel{{70,35},{1140,640}};
const sf::FloatRect closeButton{{1144,51},{44,40}};
const sf::FloatRect previousButton{{510,619},{44,38}};
const sf::FloatRect nextButton{{726,619},{44,38}};
const std::array<const char*,3> names{{"抽牌堆", "弃牌堆", "消耗牌堆"}};
const std::array<Kind,3> pileIcons{{Kind::Draw, Kind::Discard, Kind::Exhaust}};
sf::FloatRect pileBounds(int i) { return {{20.0f + 59.0f*i,621},{54,76}}; }
sf::FloatRect tabBounds(int i) { return {{97.0f + 211.0f*i,53},{201,44}}; }
sf::FloatRect cardBounds(int i) { return {{109.0f + 171.0f*(i%5),127.0f + 237.0f*(i/5)},{144,198}}; }
std::array<std::size_t,3> counts(const CombatSystem& c)
{
    return {{c.getDeck().getDrawPile().size(), c.getDeck().getDiscardPile().size(), c.getDeck().getExhaustPile().size()}};
}
void box(sf::RenderWindow& w, sf::FloatRect r, sf::Color fill, sf::Color outline = sf::Color::Transparent)
{
    sf::RectangleShape s(r.size);
    s.setPosition(r.position); s.setFillColor(fill);
    s.setOutlineThickness(1); s.setOutlineColor(outline); w.draw(s);
}
void text(sf::RenderWindow& w, const sf::Font& f, const std::string& s, unsigned size, sf::Vector2f p, sf::Color c = paper)
{
    UiHelpers::drawText(w,f,s,size,p,c);
}
void fitText(sf::RenderWindow& w, const sf::Font& f, const std::string& s, unsigned size, sf::Vector2f p, float width, sf::Color c = paper)
{
    auto t = UiHelpers::makeText(f,s,size,c);
    while (t.getLocalBounds().size.x > width && size > 10) t.setCharacterSize(--size);
    t.setPosition(p); w.draw(t);
}
void wrappedText(sf::RenderWindow& w, const sf::Font& f, const std::string& s, sf::Vector2f p, float width)
{
    sf::String line;
    auto measure = UiHelpers::makeText(f,"",17,paper);
    for (auto c : UiHelpers::toSfString(s))
    {
        sf::String candidate = line; candidate += c; measure.setString(candidate);
        if (c == U'\n' || measure.getLocalBounds().size.x > width)
        {
            measure.setString(line); measure.setPosition(p); w.draw(measure);
            p.y += 25; line.clear();
        }
        if (c != U'\n') line += c;
    }
    measure.setString(line); measure.setPosition(p); w.draw(measure);
}
void symbolButton(sf::RenderWindow& w, const sf::Font& f, sf::FloatRect r, const char* label, sf::Vector2f mouse, bool enabled = true)
{
    box(w,r, enabled && r.contains(mouse) ? sf::Color(76,82,87) : sf::Color(36,40,46), enabled ? muted : sf::Color(60,64,70));
    UiHelpers::drawCenteredText(w,f,label,24,r,enabled ? paper : sf::Color(75,81,88));
}
std::string intentLabel(const Enemy& enemy, int damage)
{
    const auto& i = enemy.getIntent();
    std::string label = i.name;
    if (i.damage > 0) label += " " + std::to_string(damage) + (i.hits > 1 ? " x " + std::to_string(i.hits) : "");
    return label;
}
Kind intentIcon(const EnemyIntent& i)
{
    if (i.damage > 0) return Kind::Attack;
    if (i.weak > 0 || i.vulnerable > 0 || i.frail > 0 || i.slimed > 0 || i.darkErosion > 0) return Kind::Weak;
    if (i.block > 0) return Kind::Guard;
    if (i.strength > 0 || i.dexterity > 0 || i.type == EnemyIntentType::Buff) return Kind::Buff;
    return Kind::Unknown;
}
}

void BattleHud::reset() { pile_ = -1; page_ = 0; mouse_ = {-1,-1}; }

bool BattleHud::handleMouseMove(sf::Vector2f p)
{
    mouse_ = p;
    if (isOpen()) return true;
    for (int i = 0; i < 3; ++i) if (pileBounds(i).contains(p)) return true;
    return false;
}

std::vector<Card> BattleHud::cards(const CombatSystem& combat) const
{
    const auto& d = combat.getDeck();
    auto result = combat.hasPendingDiscardChoice() ? combat.getDiscardChoiceCards() :
        pile_ == 0 ? d.getDrawPile() : pile_ == 1 ? d.getDiscardPile() : d.getExhaustPile();
    for (auto& card : result) card = CardPresentation::forCombat(card, combat);
    // Choice indexes must keep the exact order supplied by the combat system.
    if (combat.hasPendingDiscardChoice()) return result;
    // Browsing must neither reveal the random draw order nor mutate the live deck.
    std::stable_sort(result.begin(), result.end(), [](const Card& a, const Card& b) {
        if (a.id != b.id) return a.id < b.id;
        return a.name < b.name;
    });
    return result;
}

void BattleHud::changePage(int direction, const CombatSystem& combat)
{
    const int maxPage = std::max(0, (static_cast<int>(cards(combat).size()) - 1) / pageSize);
    page_ = std::clamp(page_ + direction, 0, maxPage);
}

bool BattleHud::handleMouseClick(sf::Vector2f p, CombatSystem& combat)
{
    mouse_ = p;
    if (combat.hasPendingDiscardChoice())
    {
        const int size = static_cast<int>(combat.getDiscardChoiceCards().size());
        page_ = std::clamp(page_, 0, std::max(0, (size - 1) / pageSize));
        for (int i = 0; i < pageSize && page_ * pageSize + i < size; ++i)
            if (cardBounds(i).contains(p))
            {
                if (combat.chooseDiscardCard(page_ * pageSize + i)) reset();
                return true;
            }
        if (previousButton.contains(p)) changePage(-1, combat);
        if (nextButton.contains(p)) changePage(1, combat);
        return true;
    }
    if (isOpen())
    {
        if (closeButton.contains(p) || !panel.contains(p)) { reset(); return true; }
        for (int i = 0; i < 3; ++i)
            if (tabBounds(i).contains(p)) { pile_ = i; page_ = 0; return true; }
        if (previousButton.contains(p)) changePage(-1, combat);
        if (nextButton.contains(p)) changePage(1, combat);
        return true;
    }
    if (combat.getResult() != BattleResult::Active) return false;
    for (int i = 0; i < 3; ++i)
        if (pileBounds(i).contains(p)) { pile_ = i; page_ = 0; return true; }
    return false;
}

bool BattleHud::handleKeyPress(sf::Keyboard::Key key, const CombatSystem& combat)
{
    if (!isOpen() && !combat.hasPendingDiscardChoice()) return false;
    if (combat.hasPendingDiscardChoice())
    {
        if (key == sf::Keyboard::Key::Left) changePage(-1, combat);
        else if (key == sf::Keyboard::Key::Right) changePage(1, combat);
        return true;
    }
    if (key == sf::Keyboard::Key::Escape) reset();
    else if (key == sf::Keyboard::Key::Left) changePage(-1, combat);
    else if (key == sf::Keyboard::Key::Right) changePage(1, combat);
    return true;
}

void BattleHud::drawStatuses(sf::RenderWindow& w, const sf::Font& f, sf::Vector2f p, int strength, int weak, int vulnerable)
{
    const std::array<int,3> values{{strength,weak,vulnerable}};
    const std::array<Kind,3> icons{{Kind::Strength,Kind::Weak,Kind::Vulnerable}};
    for (int i = 0; i < 3; ++i)
    {
        BattleIcons::draw(w,icons[i],{p.x+i*85,p.y},30);
        text(w,f,std::to_string(values[i]),19,{p.x+i*85+35,p.y+3},values[i] == 0 ? muted : paper);
    }
}

void BattleHud::drawIntent(sf::RenderWindow& w, const sf::Font& f, const Enemy& enemy, int damage)
{
    const auto& i = enemy.getIntent();
    BattleIcons::draw(w,intentIcon(i),{878,190},35);
    auto t = UiHelpers::makeText(f,intentLabel(enemy,damage),28,sf::Color(255,98,87));
    t.setStyle(sf::Text::Bold | sf::Text::Italic);
    while (t.getLocalBounds().size.x > 252 && t.getCharacterSize() > 14) t.setCharacterSize(t.getCharacterSize()-1);
    t.setOutlineColor(sf::Color(42,12,16)); t.setOutlineThickness(3);
    t.setPosition({923,188}); w.draw(t);
    if (i.damage > 0 && (i.block > 0 || i.weak > 0 || i.vulnerable > 0 || i.slimed > 0 || i.frail > 0 || i.darkErosion > 0 || i.strength > 0 || i.dexterity > 0))
        BattleIcons::draw(w,i.block > 0 ? Kind::Guard : (i.strength > 0 || i.dexterity > 0) ? Kind::Buff : Kind::Weak,{1190,193},27);
}

void BattleHud::draw(sf::RenderWindow& w, const sf::Font& f, const CombatSystem& combat) const
{
    const bool choosing = combat.hasPendingDiscardChoice();
    const auto sizes = counts(combat);
    for (int i = 0; i < 3; ++i)
    {
        const auto r = pileBounds(i);
        const bool hovered = r.contains(mouse_);
        if (hovered && !isOpen()) box(w,r,sf::Color(45,51,57,225),accent);
        BattleIcons::draw(w,pileIcons[i],r.position,48);
        auto count = UiHelpers::makeText(f,std::to_string(sizes[i]),20,paper);
        count.setStyle(sf::Text::Bold); count.setOutlineColor(sf::Color(16,18,22)); count.setOutlineThickness(2);
        count.setPosition(r.position+sf::Vector2f{32,27}); w.draw(count);
        UiHelpers::drawCenteredText(w,f,names[i],12,{r.position+sf::Vector2f{0,51},{54,20}},paper);
    }
    if (!isOpen() && !choosing)
    {
        std::string tip;
        sf::Vector2f p;
        for (int side = 0; side < 2; ++side)
            for (int i = 0; i < 3; ++i)
            {
                const sf::FloatRect r{{58.0f+820*side+85*i,150},{78,33}};
                if (r.contains(mouse_))
                {
                    const std::array<const char*,3> descriptions{{"力量：每层增加 1 点攻击伤害。负数则降低伤害。", "虚弱：攻击造成的伤害降低 25%。数字为剩余回合。", "易伤：受到的攻击伤害增加 50%。数字为剩余回合。"}};
                    tip = descriptions[i]; p = {side == 0 ? 58.0f : 874.0f,236};
                }
            }
        if (sf::FloatRect({875,185},{345,43}).contains(mouse_))
        {
            tip = intentLabel(combat.getEnemy(),combat.getEnemyIntentDamage()) + "\n" + combat.getEnemy().getIntent().description;
            p = {874,236};
        }
        for (int i = 0; i < 3; ++i)
            if (pileBounds(i).contains(mouse_)) { tip = std::string(names[i])+"  "+std::to_string(sizes[i])+" 张"; p = {20,564}; }
        if (!tip.empty()) { box(w,{p,{340,116}},sf::Color(20,23,29,248),accent); wrappedText(w,f,tip,p+sf::Vector2f{13,12},313); }
        return;
    }
    box(w,{{0,0},{1280,720}},sf::Color(0,0,0,185));
    box(w,panel,sf::Color(22,26,32,253),sf::Color(120,134,144));
    if (choosing)
    {
        text(w,f,"头槌 · 选择一张弃牌放到抽牌堆顶",27,{102,56},accent);
        text(w,f,"点击卡牌确认；之后抽牌时会优先抽到它。",17,{102,96},muted);
    }
    for (int i = 0; !choosing && i < 3; ++i)
    {
        const auto r = tabBounds(i);
        box(w,r,pile_ == i ? sf::Color(55,64,72) : sf::Color(29,34,41),pile_ == i ? accent : sf::Color(65,75,85));
        BattleIcons::draw(w,pileIcons[i],r.position+sf::Vector2f{7,5},32);
        text(w,f,std::string(names[i])+"  "+std::to_string(sizes[i]),19,r.position+sf::Vector2f{46,9});
    }
    if (!choosing) symbolButton(w,f,closeButton,"X",mouse_);
    const auto pileCards = cards(combat);
    const int pageCount = std::max(1,(static_cast<int>(pileCards.size())+pageSize-1)/pageSize);
    const int page = std::min(page_,pageCount-1);
    const int start = page*pageSize;
    const int count = std::min(pageSize,static_cast<int>(pileCards.size())-start);
    int selected = count > 0 ? 0 : -1;
    for (int i = 0; i < count; ++i)
    {
        const auto r = cardBounds(i);
        if (r.contains(mouse_)) selected = i;
        CardView card; card.setFont(f);
        // CardView scales about its unscaled centre; align the visible card with
        // the shared click/highlight rectangle after scaling.
        card.setPosition(r.position - sf::Vector2f{8,11}); card.setScale(0.9f);
        card.draw(w,pileCards[start+i]);
        fitText(w,f,pileCards[start+i].name,15,r.position+sf::Vector2f{0,202},144);
    }
    if (selected >= 0)
    {
        const auto r = cardBounds(selected);
        box(w,r,sf::Color::Transparent,accent);
        const auto& c = pileCards[start+selected];
        fitText(w,f,c.name,23,{995,140},185,accent);
        text(w,f,"费用 " + CardPresentation::costLabel(c.cost),17,{995,184},muted);
        wrappedText(w,f,c.description,{995,230},184);
    }
    else UiHelpers::drawCenteredText(w,f,"牌堆为空",25,{{120,270},{1010,100}},muted);
    symbolButton(w,f,previousButton,"<",mouse_,page > 0);
    symbolButton(w,f,nextButton,">",mouse_,page+1 < pageCount);
    UiHelpers::drawCenteredText(w,f,std::to_string(page+1)+" / "+std::to_string(pageCount),18,{{562,619},{156,38}},paper);
    fitText(w,f,choosing ? "请选择一张牌以继续战斗" : pile_ == 0 ? "按卡牌名称分组 · 非抽取顺序" : "当前战斗",14,{101,630},375,muted);
}
