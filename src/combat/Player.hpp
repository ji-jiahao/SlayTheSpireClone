#pragma once

class Player
{
public:
    Player(int maxHealth = 80, int maxEnergy = 3, int currentHealth = -1);

    void startTurn(bool retainBlock = false);
    void endTurn();
    int takeDamage(int amount);
    int loseHealth(int amount);
    int heal(int amount);
    void increaseMaxHealth(int amount);
    int gainBlock(int amount);
    int gainCardBlock(int amount);
    void gainEnergy(int amount);
    bool spendEnergy(int amount);
    void applyStrength(int amount);
    void applyWeak(int turns);
    void applyVulnerable(int turns);
    void applyFrail(int turns);
    void applyDexterity(int amount);
    void setBlockRetained(bool retained);

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
    bool getBlockRetained() const;

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
    bool blockRetained;
};
