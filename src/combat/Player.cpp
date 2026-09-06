#include "combat/Player.hpp"

#include <algorithm>

Player::Player(int maxHealth, int maxEnergy, int currentHealth)
    : maxHealth(std::max(0, maxHealth)),
      currentHealth(currentHealth < 0 ? this->maxHealth
                                      : std::clamp(currentHealth, 0, this->maxHealth)),
      maxEnergy(std::max(0, maxEnergy)),
      currentEnergy(this->maxEnergy),
      block(0), strength(0), weak(0), vulnerable(0), frail(0), dexterity(0),
      blockRetained(false)
{
}

void Player::startTurn(bool retainBlock)
{
    if (!retainBlock)
    {
        block = 0;
    }
    currentEnergy = maxEnergy;
}

void Player::endTurn()
{
    weak = std::max(0, weak - 1);
    vulnerable = std::max(0, vulnerable - 1);
    frail = std::max(0, frail - 1);
}

int Player::takeDamage(int amount)
{
    const int damage = std::max(0, amount);
    const int unblockedDamage = std::max(0, damage - block);
    block = std::max(0, block - damage);
    currentHealth = std::max(0, currentHealth - unblockedDamage);
    return unblockedDamage;
}

int Player::loseHealth(int amount)
{
    const int oldHealth = currentHealth;
    currentHealth = std::max(0, currentHealth - std::max(0, amount));
    return oldHealth - currentHealth;
}

int Player::heal(int amount)
{
    const int oldHealth = currentHealth;
    currentHealth = std::min(maxHealth, currentHealth + std::max(0, amount));
    return currentHealth - oldHealth;
}

void Player::increaseMaxHealth(int amount)
{
    const int increase = std::max(0, amount);
    maxHealth += increase;
    currentHealth += increase;
}

int Player::gainBlock(int amount)
{
    const int gained = std::max(0, amount);
    block += gained;
    return gained;
}

int Player::gainCardBlock(int amount)
{
    int gained = std::max(0, amount + dexterity);
    if (frail > 0)
    {
        gained = gained * 3 / 4;
    }
    return gainBlock(gained);
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
void Player::setBlockRetained(bool retained) { blockRetained = retained; }

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
bool Player::getBlockRetained() const { return blockRetained; }
