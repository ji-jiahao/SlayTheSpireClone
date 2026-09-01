#include "combat/Enemy.hpp"

#include <algorithm>
#include <utility>

Enemy::Enemy(std::string name, int maxHealth)
    : name(std::move(name)), maxHealth(std::max(0, maxHealth)),
      currentHealth(this->maxHealth), intentDamage(0), strength(0), weak(0), vulnerable(0)
{
}

void Enemy::takeDamage(int amount) { currentHealth = std::max(0, currentHealth - std::max(0, amount)); }
void Enemy::setIntentDamage(int amount) { intentDamage = std::max(0, amount); }
void Enemy::applyStrength(int amount) { strength += amount; }
void Enemy::applyWeak(int turns) { weak += std::max(0, turns); }
void Enemy::applyVulnerable(int turns) { vulnerable += std::max(0, turns); }

void Enemy::endTurn()
{
    weak = std::max(0, weak - 1);
    vulnerable = std::max(0, vulnerable - 1);
}

const std::string& Enemy::getName() const { return name; }
int Enemy::getCurrentHealth() const { return currentHealth; }
int Enemy::getMaxHealth() const { return maxHealth; }
int Enemy::getIntentDamage() const { return intentDamage; }
int Enemy::getStrength() const { return strength; }
int Enemy::getWeak() const { return weak; }
int Enemy::getVulnerable() const { return vulnerable; }
bool Enemy::isDead() const { return currentHealth <= 0; }
