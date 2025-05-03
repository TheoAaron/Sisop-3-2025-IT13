#include <stdio.h>
#include <string.h>
#include "dungeon.h"

Weapon weapons[MAX_WEAPONS] = {
    {1, "Terra Blade", 50, 10, ""},
    {2, "Flint & Steel", 150, 25, ""},
    {3, "Kitchen Knife", 200, 35, ""},
    {4, "Staff of Light", 120, 20, "10% Insta-Kill Chance"},
    {5, "Dragon Claws", 300, 50, "+30% Crit Chance"}};

void initialize_shop(void) {
}

Weapon *get_weapon(int id) {
    if (id < 1 || id > MAX_WEAPONS)
        return NULL;
    return &weapons[id - 1];
}