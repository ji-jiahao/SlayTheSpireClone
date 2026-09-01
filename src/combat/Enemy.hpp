#pragma once

#include <string>

class Enemy
{
public:
    Enemy(std::string name = "Cultist", int maxHealth = 40);

    void takeDamage(int amount);
    void setIntentDamage(int amount);
    void applyStrength(int amount);
    void applyWeak(int turns);
    void applyVulnerable(int turns);
    void endTurn();

    const std::string& getName() const;
    int getCurrentHealth() const;
    int getMaxHealth() const;
    int getIntentDamage() const;
    int getStrength() const;
    int getWeak() const;
    int getVulnerable() const;
    bool isDead() const;

private:
    std::string name;
    int maxHealth;
    int currentHealth;
    int intentDamage;
    int strength;
    int weak;
    int vulnerable;
};
