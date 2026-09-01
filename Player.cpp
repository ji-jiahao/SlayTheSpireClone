#include "combat/Player.hpp"

#include <algorithm>

Player::Player(int maxHealth, int maxEnergy)
    : maxHealth(std::max(0, maxHealth)),
      currentHealth(this->maxHealth),
      maxEnergy(std::max(0, maxEnergy)),
      currentEnergy(this->maxEnergy),
      block(0)
{
}

void Player::startTurn()
{
    block = 0;
    currentEnergy = maxEnergy;
}

void Player::endTurn()
{
}

void Player::takeDamage(int amount)
{
    if (amount <= 0 || currentHealth <= 0)
    {
        return;
    }

    const int unblockedDamage = std::max(0, amount - block);
    block = std::max(0, block - amount);
    currentHealth = std::max(0, currentHealth - unblockedDamage);
}

void Player::gainBlock(int amount)
{
    if (amount > 0 && currentHealth > 0)
    {
        block += amount;
    }
}

bool Player::spendEnergy(int amount)
{
    if (amount < 0 || amount > currentEnergy)
    {
        return false;
    }

    currentEnergy -= amount;
    return true;
}

int Player::getCurrentHealth() const
{
    return currentHealth;
}

int Player::getMaxHealth() const
{
    return maxHealth;
}

int Player::getBlock() const
{
    return block;
}

int Player::getCurrentEnergy() const
{
    return currentEnergy;
}

int Player::getMaxEnergy() const
{
    return maxEnergy;
}
