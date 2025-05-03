#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "dungeon.h"

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
    }
}

void show_player_stats(int sock) {
    if (send(sock, "GET_STATS", 9, 0) <= 0) {
        printf("Failed to send stats request\n");
        return;
    }

    Player player;
    ssize_t bytes_received = recv(sock, &player, sizeof(Player), 0);

    if (bytes_received <= 0) {
        printf("Failed to get player stats. Connection lost?\n");
        return;
    }
    else if (bytes_received != sizeof(Player)) {
        printf("Incomplete player data received\n");
        return;
    }

    printf("\n=== PLAYER STATS ===\n");
    printf("Gold: %d | Weapon: %s | Damage: %d | Kills: %d",
           player.gold, player.equipped_weapon, player.base_damage, player.kills);
    if (strlen(player.passive) > 0) {
        printf(" | Passive: %s", player.passive);
    }
    printf("\n");
}

void display_shop(int sock) {
    printf("\n=== WEAPON SHOP ===\n");
    for (int i = 0; i < MAX_WEAPONS; i++) {
        Weapon *w = get_weapon(i + 1);
        printf("[%d] %s - Price: %d gold, Damage: %d",
               w->id, w->name, w->price, w->damage);
        if (strlen(w->passive) > 0) {
            printf(" (Passive: %s)", w->passive);
        }
        printf("\n");
    }

    printf("\nEnter weapon number to buy (0 to cancel): ");
    int choice;
    scanf("%d", &choice);
    clear_input_buffer();

    if (choice != 0) {
        if (send(sock, "GET_STATS", 9, 0) <= 0) {
            printf("Error sending request\n");
            return;
        }

        Player player;
        int bytes_received = recv(sock, &player, sizeof(Player), 0);
        if (bytes_received <= 0) {
            printf("Failed to get player gold. Connection lost?\n");
            return;
        }
        else if (bytes_received != sizeof(Player)) {
            printf("Incomplete player data received\n");
            return;
        }

        char buy_command[32];
        snprintf(buy_command, sizeof(buy_command), "BUY %d %d", choice, player.gold);

        if (send(sock, buy_command, strlen(buy_command), 0) <= 0) {
            printf("Failed to send buy command\n");
            return;
        }

        int buy_result;
        bytes_received = recv(sock, &buy_result, sizeof(int), MSG_WAITALL);
        if (bytes_received <= 0) {
            printf("Failed to get buy result\n");
            return;
        }

        switch (buy_result) {
        case 1:
            printf("Weapon purchased successfully!\n");
            break;
        case -1:
            printf("Invalid weapon selection!\n");
            break;
        case -2:
            printf("Not enough gold! You need %d more gold.\n",
                   weapons[choice - 1].price - player.gold);
            break;
        case -3:
            printf("Inventory full!\n");
            break;
        default:
            printf("Unknown error occurred\n");
        }
    }
}

void view_inventory(int sock) {
    send(sock, "GET_INVENTORY", 13, 0);

    Player player;
    if (read(sock, &player, sizeof(Player)) <= 0) {
        printf("Failed to read inventory data\n");
        return;
    }

    printf("\n=== YOUR INVENTORY ===\n");
    if (player.inventory_size == 0) {
        printf("Your inventory is empty\n");
        return;
    }

    for (int i = 0; i < player.inventory_size; i++) {
        Weapon *w = get_weapon(player.inventory[i]);
        if (w != NULL) {
            printf("[%d] %s", w->id, w->name);
            if (strlen(w->passive) > 0) {
                printf(" (Passive: %s)", w->passive);
            }
            if (strcmp(player.equipped_weapon, w->name) == 0) {
                printf(" (EQUIPPED)");
            }
            printf("\n");
        }
    }

    printf("\nEnter weapon number to equip (0 to cancel): ");
    int choice;
    scanf("%d", &choice);

    if (choice != 0) {
        char command[20];
        sprintf(command, "EQUIP %d", choice);
        send(sock, command, strlen(command), 0);

        int result;
        read(sock, &result, sizeof(int));

        if (result == 1) {
            printf("Weapon equipped successfully!\n");
        }
        else {
            printf("Failed to equip weapon!\n");
        }
    }
}

void show_battle_menu(int sock) {
    printf("\n=== BATTLE MODE ===\n");
    
    send(sock, "GET_ENEMY", 9, 0);
    Enemy enemy;
    recv(sock, &enemy, sizeof(Enemy), 0);
    
    while (1) {
        printf("\nEnemy Health: [");
        int bars = (enemy.health * 20) / enemy.max_health;
        for (int i = 0; i < 20; i++) {
            printf(i < bars ? "#" : " ");
        }
        printf("] %d/%d\n", enemy.health, enemy.max_health);
        
        printf("\n1. Attack\n");
        printf("2. Flee\n");
        printf("Choice: ");
        
        int choice;
        scanf("%d", &choice);
        clear_input_buffer();
        
        if (choice == 1) {
            send(sock, "ATTACK", 6, 0);
            
            int result;
            recv(sock, &result, sizeof(int), 0);
            recv(sock, &enemy, sizeof(Enemy), 0);
            
            if (result > 0) {
                printf("\nYou defeated the enemy and earned %d gold!\n", result);
                printf("A new enemy appears!\n");
            } else {
                printf("\nYou dealt %d damage to the enemy!\n", -result);
            }
        } 
        else if (choice == 2) {
            printf("\nYou fled from battle!\n");
            break;
        }
        else {
            printf("\nInvalid choice!\n");
        }
    }
}

int connect_to_server() {
    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    return sock;
}

int main() {
    int sock = connect_to_server();
    if (sock < 0) {
        return 1;
    }

    initialize_shop();

    int choice;
    do {
        printf("\n==== MAIN MENU ====\n");
        printf("1. Show Player Stats\n");
        printf("2. Shop\n");
        printf("3. Inventory & Equip Weapon\n");
        printf("4. Battle\n");
        printf("5. Exit\n");
        printf("Choice: ");

        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            printf("Invalid input\n");
            continue;
        }
        clear_input_buffer();

        switch (choice) {
        case 1:
            show_player_stats(sock);
            break;
        case 2:
            display_shop(sock);
            break;
        case 3:
            view_inventory(sock);
            break;
        case 4:
            show_battle_menu(sock);
            break;
        case 5:
            printf("Goodbye!\n");
            break;
        default:
            printf("Invalid choice\n");
        }
    } while (choice != 5);

    close(sock);
    return 0;
}