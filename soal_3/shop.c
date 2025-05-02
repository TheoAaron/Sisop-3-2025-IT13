#include "shop.h"

Weapon *get_weapon_list(int *count)
{
    static Weapon weapons[] = {
        {"Classic", 26, 100, "Crit"},
        {"Ghost", 30, 300, "Crit"},
        {"Sheriff", 55, 500, "Crit"},
        {"Frenzy", 26, 250, "Crit"},
        {"Shorty", 24, 150, "Crit"},
        {"Stinger", 27, 950, "Crit"},
        {"Spectre", 26, 1600, "Crit"},
        {"Bucky", 20, 850, "Crit"},
        {"Judge", 34, 1850, "Crit"},
        {"Bulldog", 35, 2050, "Crit"},
        {"Guardian", 65, 2250, "Crit"},
        {"Phantom", 39, 2900, "Crit"},
        {"Vandal", 40, 2900, "Crit"},
        {"Marshal", 101, 950, "Crit"},
        {"Operator", 150, 4700, "Crit"},
        {"Melee", 50, 0, "Crit"}
    };
    *count = sizeof(weapons) / sizeof(weapons[0]);
    return weapons;
}
