#include "combat/Enemy.hpp"

#include <algorithm>
#include <utility>

Enemy::Enemy(std::string name, int maxHealth)
    : name(std::move(name)),
      maxHealth(std::max(0, maxHealth)),
      currentHealth(this->maxHealth),
      intentDamage(0)
{
}

void Enemy::takeDamage(int amount)
{
    if (amount > 0 && !isDead())
    {
        currentHealth = std::max(0, currentHealth - amount);
    }
}

void Enemy::setIntentDamage(int amount)
{
    intentDamage = std::max(0, amount);
}

const std::string& Enemy::getName() const
{
    return name;
}

int Enemy::getCurrentHealth() const
{
    return currentHealth;
}

int Enemy::getMaxHealth() const
{
    return maxHealth;
}

int Enemy::getIntentDamage() const
{
    return intentDamage;
}

bool Enemy::isDead() const
{
    return currentHealth <= 0;
}
