#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dungeon.h"

Player player = {
    .gold = 500,
    .equipped_weapon = "Fists",
    .base_damage = 5,
    .kills = 0,
    .passive = "",
    .inventory = {0},
    .inventory_size = 0};

Enemy generate_random_enemy(void) {
    Enemy enemy;
    enemy.max_health = MIN_ENEMY_HEALTH + rand() % (MAX_ENEMY_HEALTH - MIN_ENEMY_HEALTH + 1);
    enemy.health = enemy.max_health;
    return enemy;
}

void send_enemy_data(int socket, Enemy *enemy) {
    if (send(socket, enemy, sizeof(Enemy), 0) <= 0)
    {
        perror("Failed to send enemy data");
    }
}

void send_player_data(int socket, Player *player) {
    if (send(socket, player, sizeof(Player), 0) <= 0)
    {
        perror("Failed to send player data");
    }
}

void send_inventory_data(int socket, Player *player) {
    if (send(socket, player, sizeof(Player), 0) <= 0)
    {
        perror("Failed to send inventory data");
    }
}

int calculate_damage(Player *player) {
    int base_damage = player->base_damage;
    int damage = base_damage * (0.8 + (rand() % 41) / 100.0);

    if (check_critical_hit()) {
        damage *= 2;
    }

    return damage;
}

bool check_critical_hit(void) {
    return (rand() % 100) < 20;
}

bool check_instant_kill(Player *player) {
    if (strstr(player->passive, "Insta-Kill") != NULL)
    {
        return (rand() % 100) < 10;
    }
    return false;
}

int handle_attack_command(int socket, Player* player, Enemy* enemy) {
    int damage = calculate_damage(player);
    
    if (check_instant_kill(player)) {
        damage = enemy->health;
        printf("Instant kill activated!\n");
    }
    
    enemy->health -= damage;
    
    if (enemy->health <= 0) {
        int reward = MIN_REWARD + rand() % (MAX_REWARD - MIN_REWARD + 1);
        player->gold += reward;
        player->kills++;
        *enemy = generate_random_enemy();
        return reward;
    }
    
    return -damage;
}

int handle_equip_command(int socket, int weapon_id) {
    for (int i = 0; i < player.inventory_size; i++) {
        if (player.inventory[i] == weapon_id) {
            Weapon *w = get_weapon(weapon_id);
            if (w != NULL) {
                strncpy(player.equipped_weapon, w->name, WEAPON_NAME_LEN);
                player.base_damage = w->damage;
                strncpy(player.passive, w->passive, PASSIVE_LEN);
                return 1;
            }
        }
    }
    return -1;
}

int handle_buy_command(int socket, int weapon_id, int player_gold) {
    Weapon *w = get_weapon(weapon_id);
    if (!w)
        return -1;
    if (player_gold < w->price)
        return -2;
    if (player.inventory_size >= MAX_INVENTORY)
        return -3;

    player.inventory[player.inventory_size++] = weapon_id;
    player.gold -= w->price;
    return 1;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    Enemy current_enemy = generate_random_enemy();

    while ((bytes_read = recv(client_socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';

        if (strncmp(buffer, "GET_STATS", 9) == 0) {
            send_player_data(client_socket, &player);
        }
        else if (strncmp(buffer, "GET_INVENTORY", 13) == 0) {
            send_inventory_data(client_socket, &player);
        }
        else if (strncmp(buffer, "GET_ENEMY", 9) == 0) {
            send_enemy_data(client_socket, &current_enemy);
        }
        else if (strncmp(buffer, "ATTACK", 6) == 0) {
            int result = handle_attack_command(client_socket, &player, &current_enemy);
            send(client_socket, &result, sizeof(int), 0);
            send_enemy_data(client_socket, &current_enemy);
        }
        else if (strncmp(buffer, "BUY ", 4) == 0) {
            int weapon_id, player_gold;
            if (sscanf(buffer + 4, "%d %d", &weapon_id, &player_gold) == 2) {
                int result = handle_buy_command(client_socket, weapon_id, player_gold);
                send(client_socket, &result, sizeof(int), 0);
            }
        }
        else if (strncmp(buffer, "EQUIP ", 6) == 0) {
            int weapon_id = atoi(buffer + 6);
            int result = handle_equip_command(client_socket, weapon_id);
            send(client_socket, &result, sizeof(int), 0);
        }
    }
    close(client_socket);
}

int start_server() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            continue;
        }
        handle_client(new_socket);
    }
    return 0;
}

int main() {
    initialize_shop();
    return start_server();
}