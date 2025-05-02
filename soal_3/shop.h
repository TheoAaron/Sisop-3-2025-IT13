#ifndef COMMON_H
#define COMMON_H

#define MAX_WEAPON 10

typedef struct {
    char name[32];
    int damage;
    int price;
    char passive[64];
} Weapon;

typedef struct {
    int gold;
    int base_damage;
    Weapon inventory[MAX_WEAPON];
    int inventory_count;
    int current_weapon;
    int monsters_defeated;
} Player;

typedef struct {
    int hp;
    int max_hp;
} Enemy;


#endif
