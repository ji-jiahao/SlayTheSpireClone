#pragma once

#include <string>

class Enemy
{
public:
    Enemy(std::string name, int maxHealth);

    void takeDamage(int amount);
    void setIntentDamage(int amount);

    const std::string& getName() const;
    int getCurrentHealth() const;
    int getMaxHealth() const;
    int getIntentDamage() const;
    bool isDead() const;

private:
    std::string name;
    int maxHealth;
    int currentHealth;
    int intentDamage;
};
