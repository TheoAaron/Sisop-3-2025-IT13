#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "shop.h"

#define FIFO_SERVER "dungeon.pipe"

int main()
{
    mkfifo(FIFO_SERVER, 0666);
    pid_t pid = getpid();

    char fifo_reply[64];
    sprintf(fifo_reply, "player_%d.pipe", pid);
    mkfifo(fifo_reply, 0666);

    while (1)
    {
        printf("\n=== Main Menu ===\n");
        printf("1. Show Player Stats\n");
        printf("2. Shop\n");
        printf("3. Open Inventory & Equip Weapon\n");
        printf("4. Battle Mode\n");
        printf("5. Exit Dungeon\n");
        printf("Choose: ");

        int choice;
        scanf("%d", &choice);
        getchar();

        char cmd[64];
        if (choice == 1)
            strcpy(cmd, "stats");
        else if (choice == 2) {
            strcpy(cmd, "shop");
        
            FILE *fw = fopen(FIFO_SERVER, "w");
            fprintf(fw, "%d:%s\n", pid, cmd);
            fclose(fw);
        
            FILE *fr = fopen(fifo_reply, "r");
            char buffer[1024];
            fread(buffer, 1, sizeof(buffer), fr);
            fclose(fr);
            printf("%s", buffer);
        
            printf("Input your choice: ");
            int shop_choice;
            scanf("%d", &shop_choice);
            getchar();
        
            char buy_fifo[64];
            sprintf(buy_fifo, "buy_input_%d.pipe", pid);
            FILE *fw2 = fopen(buy_fifo, "w");
            fprintf(fw2, "%d\n", shop_choice);
            fclose(fw2);
        
            FILE *fr2 = fopen(fifo_reply, "r");
            char result[512];
            fread(result, 1, sizeof(result), fr2);
            fclose(fr2);
            printf("%s\n", result);
        }                
        else if (choice == 3) {
            strcpy(cmd, "inventory");

            FILE *fw = fopen(FIFO_SERVER, "w");
            fprintf(fw, "%d:%s\n", pid, cmd);
            fclose(fw);
            
            FILE *fr = fopen(fifo_reply, "r");
            char buffer[1024];
            fread(buffer, 1, sizeof(buffer), fr);
            fclose(fr);
            printf("%s", buffer);
        
            printf("Input your choice: ");
            int weapon_choice;
            scanf("%d", &weapon_choice);
            getchar();
            
            char input_fifo[64];
            sprintf(input_fifo, "equip_input_%d.pipe", pid);
            FILE *fw2 = fopen(input_fifo, "w");
            fprintf(fw2, "%d\n", weapon_choice);
            fclose(fw2);
            
            FILE *fr2 = fopen(fifo_reply, "r");
            char final[512];
            fread(final, 1, sizeof(final), fr2);
            fclose(fr2);
            printf("%s\n", final);
        }            
        else if (choice == 4)
            strcpy(cmd, "battle");
            else if (choice == 3) {
            FILE *fw = fopen(FIFO_SERVER, "w");
            if (fw) {
                fprintf(fw, "%d:exit\n", pid);
                fclose(fw);
            }
        
            printf("Exiting dungeon. Goodbye, adventurer!\n");
        
            unlink(fifo_reply);
            break;
        }
        else if (choice == 5) {
            FILE *fw = fopen(FIFO_SERVER, "w");
            if (fw) {
                fprintf(fw, "%d:exit\n", pid);
                fclose(fw);
            }
        
            printf("Exiting dungeon. Goodbye, adventurer!\n");
        
            unlink(fifo_reply);
            break;
        }
        else
        {
            printf("Invalid input.\n");
            continue;
        }

        FILE *fw = fopen(FIFO_SERVER, "w");
        if (!fw)
        {
            perror("Error opening server pipe");
            continue;
        }

        fprintf(fw, "%d:%s\n", pid, cmd);
        fclose(fw);

        FILE *fr = fopen(fifo_reply, "r");
        if (fr)
        {
            char response[512];
            fread(response, 1, sizeof(response), fr);
            printf("%s\n", response);
            fclose(fr);
        }
    }

    unlink(fifo_reply);
    return 0;
}
