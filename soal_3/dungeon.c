#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "shop.h"

#define FIFO_SERVER "dungeon.pipe"

Player player_data;

void init_player()
{
    player_data.gold = 500;
    player_data.base_damage = 10;
    player_data.inventory_count = 1;
    strcpy(player_data.inventory[0].name, "Wooden Sword");
    player_data.inventory[0].damage = 10;
    player_data.inventory[0].price = 50;
    strcpy(player_data.inventory[0].passive, "-");
    player_data.current_weapon = 0;
    player_data.monsters_defeated = 0;
}

void show_stats(char *resp)
{
    Weapon *w = &player_data.inventory[player_data.current_weapon];
    sprintf(resp, "Gold: %d\nBase Damage: %d\nWeapon: %s\nPassive: %s\nMonsters Defeated: %d\n",
            player_data.gold, player_data.base_damage, w->name, w->passive, player_data.monsters_defeated);
}

void handle_shop(char *resp)
{
    extern Weapon *get_weapon_list(int *count);
    int count;
    Weapon *list = get_weapon_list(&count);
    strcpy(resp, "=== Shop ===\n");
    for (int i = 0; i < count; i++)
    {
        char line[256];
        sprintf(line, "%d. %s (DMG: %d, Price: %d, Passive: %s)\n", i + 1, list[i].name, list[i].damage, list[i].price, list[i].passive);
        strcat(resp, line);
    }
}

void handle_shop_purchase(char *resp, int pid) {
    extern Weapon* get_weapon_list(int *count);
    int count;
    Weapon *list = get_weapon_list(&count);

    char result[1024] = "=== Shop ===\n";
    for (int i = 0; i < count; i++) {
        char line[256];
        sprintf(line, "%d. %s (DMG: %d, Price: %d, Passive: %s)\n", i + 1,
                list[i].name, list[i].damage, list[i].price, list[i].passive);
        strcat(result, line);
    }

    strcat(result, "\nEnter weapon number to buy or 0 to cancel:\n");
    strcpy(resp, result);

    char buy_fifo[64];
    sprintf(buy_fifo, "buy_input_%d.pipe", pid);
    mkfifo(buy_fifo, 0666);

    FILE *fr = fopen(buy_fifo, "r");
    if (fr) {
        char buf[16];
        fgets(buf, sizeof(buf), fr);
        int choice = atoi(buf);
        fclose(fr);
        unlink(buy_fifo);

        if (choice <= 0 || choice > count) {
            strcpy(resp, "Purchase canceled.\n");
            return;
        }

        Weapon selected = list[choice - 1];
        if (player_data.gold < selected.price) {
            sprintf(resp, "You don't have enough gold to buy %s.\n", selected.name);
            return;
        }

        if (player_data.inventory_count >= MAX_WEAPON) {
            strcpy(resp, "Your inventory is full!\n");
            return;
        }

        player_data.gold -= selected.price;
        player_data.inventory[player_data.inventory_count++] = selected;
        sprintf(resp, "You bought %s!\n", selected.name);
    } else {
        strcpy(resp, "Failed to read input.\n");
    }
}

void handle_inventory_equip(char *resp, int pid) {
    char result[1024] = "=== Inventory ===\n";
    if (player_data.inventory_count == 0) {
        strcat(result, "(empty)\n");
        strcpy(resp, result);
        return;
    }

    for (int i = 0; i < player_data.inventory_count; i++) {
        char line[128];
        sprintf(line, "%d. %s (DMG: %d, Passive: %s)%s\n", i + 1,
                player_data.inventory[i].name,
                player_data.inventory[i].damage,
                player_data.inventory[i].passive,
                i == player_data.current_weapon ? " [EQUIPPED]" : "");
        strcat(result, line);
    }

    strcat(result, "\nChoose weapon number to equip or 0 to cancel:\n");
    strcpy(resp, result);

    char input_fifo[64];
    sprintf(input_fifo, "equip_input_%d.pipe", pid);
    mkfifo(input_fifo, 0666);

    FILE *fr = fopen(input_fifo, "r");
    if (fr) {
        char buf[16];
        fgets(buf, sizeof(buf), fr);
        int choice = atoi(buf);
        fclose(fr);
        unlink(input_fifo);

        if (choice > 0 && choice <= player_data.inventory_count) {
            player_data.current_weapon = choice - 1;
            sprintf(resp, "Equipped %s.\n", player_data.inventory[choice - 1].name);
        } else {
            strcat(resp, "No weapon equipped.\n");
        }
    } else {
        strcat(resp, "Failed to read input.\n");
    }
}

Enemy spawn_enemy() {
    Enemy e;
    e.max_hp = 50 + rand() % 151;
    e.hp = e.max_hp;
    return e;
}

int calculate_damage(Player *p, int *critical, int *passive_triggered) {
    Weapon *w = &p->inventory[p->current_weapon];
    int dmg = w->damage;
    *critical = 0;
    *passive_triggered = 0;

    if (strcmp(w->passive, "Crit") == 0 && (rand() % 5 == 0)) {
        dmg = (int)(dmg * 1.5);
        *critical = 1;
        *passive_triggered = 1;
    }

    return dmg;
}

void handle_battle(char *resp) {
    Enemy e = spawn_enemy();
    char log[512] = "";
    sprintf(log, "Enemy appears! HP: %d\n", e.hp);

    while (e.hp > 0) {
        int critical = 0;
        int passive_triggered = 0;
        int dmg = calculate_damage(&player_data, &critical, &passive_triggered);
        e.hp -= dmg;
        if (e.hp < 0) e.hp = 0;

        char turn[128];
        sprintf(turn, "You dealt %d damage%s. Enemy HP: %d/%d\n",
                dmg, critical ? " (CRITICAL!)" : "", e.hp, e.max_hp);
        strcat(log, turn);

        if (passive_triggered) {
            if (critical)
                strcat(log, "Passive activated: HEADSHOT!\n");
            else
                strcat(log, "Passive activated: LEGSHOT!\n");
        }        

        if (e.hp <= 0) break;
    }

    player_data.monsters_defeated++;
    int reward = 50 + rand() % 51;
    player_data.gold += reward;

    char reward_msg[64];
    sprintf(reward_msg, "Enemy defeated! You gained %d gold.\n", reward);
    strcat(log, reward_msg);

    strcpy(resp, log);
}

void handle_command(const char *cmd, char *resp, int pid)
{
    if (strcmp(cmd, "stats") == 0)
    {
        show_stats(resp);
    }
    else if (strcmp(cmd, "shop") == 0) {
        handle_shop_purchase(resp, pid);
    }
    else if (strcmp(cmd, "inventory") == 0) {
        handle_inventory_equip(resp, pid);
    }
    else if (strcmp(cmd, "battle") == 0)
    {
        handle_battle(resp);
    }
    else if (strcmp(cmd, "exit") == 0) {
        sprintf(resp, "Player %d exited the dungeon.\n", pid);
    }       
    else
    {
        sprintf(resp, "Unknown command: %s", cmd);
    }
}

int main()
{
    mkfifo(FIFO_SERVER, 0666);
    init_player();

    while (1)
    {
        FILE *fp = fopen(FIFO_SERVER, "r");
        if (!fp)
            continue;

        char buffer[256];
        if (!fgets(buffer, sizeof(buffer), fp))
        {
            fclose(fp);
            continue;
        }
        fclose(fp);

        char *pid_str = strtok(buffer, ":");
        int pid = atoi(pid_str);
        char *cmd = strtok(NULL, "\n");

        char response[512];
        handle_command(cmd, response, pid);

        char reply_fifo[64];
        sprintf(reply_fifo, "player_%d.pipe", pid);
        FILE *fw = fopen(reply_fifo, "w");
        if (fw)
        {
            fputs(response, fw);
            fclose(fw);
        }
    }

    return 0;
}
