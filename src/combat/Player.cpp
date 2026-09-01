#include "combat/Player.hpp"

#include <algorithm>

Player::Player(int maxHealth, int maxEnergy, int currentHealth)
    : maxHealth(std::max(0, maxHealth)),
      currentHealth(currentHealth < 0 ? this->maxHealth
                                      : std::clamp(currentHealth, 0, this->maxHealth)),
      maxEnergy(std::max(0, maxEnergy)),
      currentEnergy(this->maxEnergy),
      block(0), strength(0), weak(0), vulnerable(0), frail(0), dexterity(0)
{
}

void Player::startTurn()
{
    block = 0;
    currentEnergy = maxEnergy;
}

void Player::endTurn()
{
    weak = std::max(0, weak - 1);
    vulnerable = std::max(0, vulnerable - 1);
    frail = std::max(0, frail - 1);
}

void Player::takeDamage(int amount)
{
    const int damage = std::max(0, amount);
    const int unblockedDamage = std::max(0, damage - block);
    block = std::max(0, block - damage);
    currentHealth = std::max(0, currentHealth - unblockedDamage);
}

void Player::loseHealth(int amount)
{
    currentHealth = std::max(0, currentHealth - std::max(0, amount));
}

void Player::gainBlock(int amount) { block += std::max(0, amount); }
void Player::gainCardBlock(int amount)
{
    int gained = std::max(0, amount + dexterity);
    if (frail > 0) gained = gained * 3 / 4;
    gainBlock(gained);
}
void Player::gainEnergy(int amount) { currentEnergy += std::max(0, amount); }

bool Player::spendEnergy(int amount)
{
    if (amount < 0 || amount > currentEnergy) return false;
    currentEnergy -= amount;
    return true;
}

void Player::applyStrength(int amount) { strength += amount; }
void Player::applyWeak(int turns) { weak += std::max(0, turns); }
void Player::applyVulnerable(int turns) { vulnerable += std::max(0, turns); }
void Player::applyFrail(int turns) { frail += std::max(0, turns); }
void Player::applyDexterity(int amount) { dexterity += amount; }

int Player::getCurrentHealth() const { return currentHealth; }
int Player::getMaxHealth() const { return maxHealth; }
int Player::getBlock() const { return block; }
int Player::getCurrentEnergy() const { return currentEnergy; }
int Player::getMaxEnergy() const { return maxEnergy; }
int Player::getStrength() const { return strength; }
int Player::getWeak() const { return weak; }
int Player::getVulnerable() const { return vulnerable; }
int Player::getFrail() const { return frail; }
int Player::getDexterity() const { return dexterity; }
