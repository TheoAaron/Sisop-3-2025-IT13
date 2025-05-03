#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>      // O_CREAT, O_RDWR
#include <sys/stat.h>   // mode constants
#include <sys/mman.h>   // shm_open, mmap
#include <unistd.h>     // ftruncate, close
#include <semaphore.h>

#define MAX_HUNTERS   100
#define MAX_DUNGEONS   50

typedef struct {
    int used;
    char name[32];
    int key;
    int level, exp, atk, hp, def;
    int banned;
    int notify;
} hunter_t;

typedef struct {
    int used;
    char name[32];
    int min_level;
    int reward_atk, reward_hp, reward_def, reward_exp;
    int key;
} dungeon_t;

const char *HUNTER_SHM = "/hunters_shm";
const char *DUNGEON_SHM = "/dungeons_shm";
const char *HUNTER_SEM = "/hunters_sem";
const char *DUNGEON_SEM = "/dungeons_sem";

hunter_t   *hunters;
dungeon_t  *dungeons;
sem_t      *hsem, *dsem;

void init_shared() {
    int hfd = shm_open(HUNTER_SHM, O_CREAT|O_RDWR, 0666);
    int dfd = shm_open(DUNGEON_SHM, O_CREAT|O_RDWR, 0666);
    if (hfd < 0 || dfd < 0) { perror("shm_open"); exit(1); }

    ftruncate(hfd, sizeof(hunter_t)*MAX_HUNTERS);
    ftruncate(dfd, sizeof(dungeon_t)*MAX_DUNGEONS);

    hunters = mmap(NULL, sizeof(hunter_t)*MAX_HUNTERS,
                   PROT_READ|PROT_WRITE, MAP_SHARED, hfd, 0);
    dungeons= mmap(NULL, sizeof(dungeon_t)*MAX_DUNGEONS,
                   PROT_READ|PROT_WRITE, MAP_SHARED, dfd, 0);
    if (hunters==MAP_FAILED || dungeons==MAP_FAILED) { perror("mmap"); exit(1); }

    // init to zero
    memset(hunters, 0, sizeof(hunter_t)*MAX_HUNTERS);
    memset(dungeons,0, sizeof(dungeon_t)*MAX_DUNGEONS);

    close(hfd); close(dfd);

    // create semaphores
    sem_unlink(HUNTER_SEM);
    sem_unlink(DUNGEON_SEM);
    hsem = sem_open(HUNTER_SEM, O_CREAT, 0666, 1);
    dsem = sem_open(DUNGEON_SEM, O_CREAT, 0666, 1);
    if (hsem==SEM_FAILED||dsem==SEM_FAILED) { perror("sem_open"); exit(1); }

    srand(time(NULL));
    printf("Shared memory & semaphore initialized.\n");
}

void cleanup() {
    munmap(hunters, sizeof(hunter_t)*MAX_HUNTERS);
    munmap(dungeons,sizeof(dungeon_t)*MAX_DUNGEONS);
    shm_unlink(HUNTER_SHM);
    shm_unlink(DUNGEON_SHM);
    sem_close(hsem);
    sem_close(dsem);
    sem_unlink(HUNTER_SEM);
    sem_unlink(DUNGEON_SEM);
    printf("Cleaned up shared memory & semaphores.\n");
}

void list_hunters() {
    sem_wait(hsem);
    printf("=== Registered Hunters ===\n");
    for (int i = 0; i < MAX_HUNTERS; i++) {
        if (!hunters[i].used) continue;
        printf("Name: %s | Key: %d | Lvl:%d | EXP:%d | ATK:%d | HP:%d | DEF:%d | %s\n",
               hunters[i].name, hunters[i].key,
               hunters[i].level, hunters[i].exp,
               hunters[i].atk, hunters[i].hp, hunters[i].def,
               hunters[i].banned ? "BANNED":"active");
    }
    sem_post(hsem);
}

void list_dungeons() {
    sem_wait(dsem);
    printf("=== Available Dungeons ===\n");
    for (int i = 0; i < MAX_DUNGEONS; i++) {
        if (!dungeons[i].used) continue;
        printf("Name: %s | Key: %d | MinLvl:%d | Rewards:(EXP %d, ATK %d, HP %d, DEF %d)\n",
               dungeons[i].name, dungeons[i].key,
               dungeons[i].min_level,
               dungeons[i].reward_exp, dungeons[i].reward_atk,
               dungeons[i].reward_hp, dungeons[i].reward_def);
    }
    sem_post(dsem);
}

void ban_unban(int key, int flag) {
    sem_wait(hsem);
    for (int i = 0; i < MAX_HUNTERS; i++) {
        if (hunters[i].used && hunters[i].key == key) {
            hunters[i].banned = flag;
            printf("Hunter key %d is now %s\n", key, flag?"BANNED":"UNBANNED");
            sem_post(hsem);
            return;
        }
    }
    sem_post(hsem);
    printf("Key %d not found.\n", key);
}

void reset_stats(int key) {
    sem_wait(hsem);
    for (int i = 0; i < MAX_HUNTERS; i++) {
        if (hunters[i].used && hunters[i].key == key) {
            hunters[i].level = 1;
            hunters[i].exp   = 0;
            hunters[i].atk   = 10;
            hunters[i].hp    = 100;
            hunters[i].def   = 5;
            printf("Stats for key %d reset to initial.\n", key);
            sem_post(hsem);
            return;
        }
    }
    sem_post(hsem);
    printf("Key %d not found.\n", key);
}

void generate_dungeon() {
    sem_wait(dsem);
    for (int i = 0; i < MAX_DUNGEONS; i++) {
        if (!dungeons[i].used) {
            dungeons[i].used = 1;
            snprintf(dungeons[i].name,32,"Dungeon_%d", rand()%1000);
            dungeons[i].key = rand();
            dungeons[i].min_level = rand()%5 + 1;
            dungeons[i].reward_atk = rand()%51 + 100;   //100–150
            dungeons[i].reward_hp  = rand()%51 + 50;    //50–100
            dungeons[i].reward_def = rand()%26 + 25;    //25–50
            dungeons[i].reward_exp = rand()%151 +150;   //150–300
            printf("Generated %s (key %d)\n",
                   dungeons[i].name, dungeons[i].key);
            sem_post(dsem);
            return;
        }
    }
    sem_post(dsem);
    printf("Dungeon list penuh!\n");
}

int main(){
    atexit(cleanup);
    init_shared();

    char cmd[64];
    while (1) {
        printf("\n[admin] Enter command (list_h list_d ban unban reset gen exit): ");
        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        int key;
        if (sscanf(cmd,"ban %d",&key)==1) ban_unban(key,1);
        else if (sscanf(cmd,"unban %d",&key)==1) ban_unban(key,0);
        else if (sscanf(cmd,"reset %d",&key)==1) reset_stats(key);
        else if (strncmp(cmd,"list_h",6)==0) list_hunters();
        else if (strncmp(cmd,"list_d",6)==0) list_dungeons();
        else if (strncmp(cmd,"gen",3)==0) generate_dungeon();
        else if (strncmp(cmd,"exit",4)==0) break;
        else printf("Unknown.\n");
    }
    return 0;
}
