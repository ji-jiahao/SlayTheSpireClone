#pragma once

class Player
{
public:
    Player(int maxHealth, int maxEnergy);

    void startTurn();
    void endTurn();
    void takeDamage(int amount);
    void gainBlock(int amount);
    bool spendEnergy(int amount);

    int getCurrentHealth() const;
    int getMaxHealth() const;
    int getBlock() const;
    int getCurrentEnergy() const;
    int getMaxEnergy() const;

private:
    int maxHealth;
    int currentHealth;
    int maxEnergy;
    int currentEnergy;
    int block;
};
