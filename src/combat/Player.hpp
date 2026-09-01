#pragma once

class Player
{
public:
    Player(int maxHealth = 80, int maxEnergy = 3, int currentHealth = -1);

    void startTurn();
    void endTurn();
    void takeDamage(int amount);
    void loseHealth(int amount);
    void gainBlock(int amount);
    void gainCardBlock(int amount);
    void gainEnergy(int amount);
    bool spendEnergy(int amount);
    void applyStrength(int amount);
    void applyWeak(int turns);
    void applyVulnerable(int turns);
    void applyFrail(int turns);
    void applyDexterity(int amount);

    int getCurrentHealth() const;
    int getMaxHealth() const;
    int getBlock() const;
    int getCurrentEnergy() const;
    int getMaxEnergy() const;
    int getStrength() const;
    int getWeak() const;
    int getVulnerable() const;
    int getFrail() const;
    int getDexterity() const;

private:
    int maxHealth;
    int currentHealth;
    int maxEnergy;
    int currentEnergy;
    int block;
    int strength;
    int weak;
    int vulnerable;
    int frail;
    int dexterity;
};
