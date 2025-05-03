#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>

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

hunter_t  *me = NULL;
pthread_t notif_thread;
int notif_running = 0;

void *notif_func(void *arg) {
    while (me->notify) {
        sem_wait(dsem);
        printf("\n--- NOTIF: Dungeons available ---\n");
        for (int i=0;i<MAX_DUNGEONS;i++){
            if (dungeons[i].used && dungeons[i].min_level<=me->level){
                printf("%s (key %d, minLvl %d)\n",
                    dungeons[i].name, dungeons[i].key, dungeons[i].min_level);
            }
        }
        sem_post(dsem);
        sleep(3);
    }
    return NULL;
}

int register_hunter() {
    char name[32];
    printf("Choose name: "); scanf("%31s",name);
    sem_wait(hsem);
    for (int i=0;i<MAX_HUNTERS;i++){
        if (!hunters[i].used){
            hunters[i].used = 1;
            strncpy(hunters[i].name,name,31);
            hunters[i].key   = rand();
            hunters[i].level = 1;
            hunters[i].exp   = 0;
            hunters[i].atk   = 10;
            hunters[i].hp    = 100;
            hunters[i].def   = 5;
            hunters[i].banned= 0;
            hunters[i].notify= 0;
            printf("Registered! Your key: %d\n", hunters[i].key);
            sem_post(hsem);
            return hunters[i].key;
        }
    }
    sem_post(hsem);
    printf("Server penuh.\n");
    return -1;
}

hunter_t *login_hunter() {
    int key; printf("Enter your key: "); scanf("%d",&key);
    sem_wait(hsem);
    for (int i=0;i<MAX_HUNTERS;i++){
        if (hunters[i].used && hunters[i].key==key){
            if (hunters[i].banned) {
                printf("You are banned!\n");
                sem_post(hsem);
                return NULL;
            }
            printf("Welcome back, %s!\n", hunters[i].name);
            sem_post(hsem);
            return &hunters[i];
        }
    }
    sem_post(hsem);
    printf("Key tidak ditemukan.\n");
    return NULL;
}

void view_dungeons() {
    sem_wait(dsem);
    printf("=== Dungeons for Level %d ===\n", me->level);
    for (int i=0;i<MAX_DUNGEONS;i++){
        if (dungeons[i].used && dungeons[i].min_level<=me->level){
            printf("%s (key %d) Rewards: EXP%d ATK%d HP%d DEF%d\n",
                dungeons[i].name, dungeons[i].key,
                dungeons[i].reward_exp,dungeons[i].reward_atk,
                dungeons[i].reward_hp,dungeons[i].reward_def);
        }
    }
    sem_post(dsem);
}

void raid_dungeon() {
    int key; printf("Dungeon key to raid: "); scanf("%d",&key);
    sem_wait(dsem);
    for (int i=0;i<MAX_DUNGEONS;i++){
        if (dungeons[i].used && dungeons[i].key==key &&
            dungeons[i].min_level<=me->level) {
            // apply rewards
            me->atk += dungeons[i].reward_atk;
            me->hp  += dungeons[i].reward_hp;
            me->def += dungeons[i].reward_def;
            me->exp += dungeons[i].reward_exp;
            dungeons[i].used = 0;
            printf("Dungeon conquered! Rewards applied.\n");
            // level up?
            if (me->exp>=500){
                me->level++;
                me->exp=0;
                printf("Level UP! Now Lvl %d\n", me->level);
            }
            sem_post(dsem);
            return;
        }
    }
    sem_post(dsem);
    printf("Dungeon tidak valid atau level terlalu rendah.\n");
}

void battle_hunter() {
    int key; printf("Opponent key: "); scanf("%d",&key);
    sem_wait(hsem);
    hunter_t *opp = NULL;
    for (int i=0;i<MAX_HUNTERS;i++){
        if (hunters[i].used && hunters[i].key==key) opp=&hunters[i];
    }
    if (!opp || opp->banned || opp==me) {
        printf("Invalid opponent.\n");
        sem_post(hsem);
        return;
    }
    int pow_me = me->atk + me->hp + me->def;
    int pow_op = opp->atk + opp->hp + opp->def;
    if (pow_me >= pow_op) {
        me->atk += opp->atk;
        me->hp  += opp->hp;
        me->def += opp->def;
        opp->used = 0;
        printf("You win! Stats stolen.\n");
    } else {
        opp->atk += me->atk;
        opp->hp  += me->hp;
        opp->def += me->def;
        me->used = 0;
        printf("You lose... you're dead.\n");
        sem_post(hsem);
        exit(0);
    }
    sem_post(hsem);
}

int main(){
    srand(time(NULL));
    // open shared memory & semaphores
    int hfd = shm_open(HUNTER_SHM, O_RDWR, 0666);
    int dfd = shm_open(DUNGEON_SHM, O_RDWR, 0666);
    hunters = mmap(NULL, sizeof(hunter_t)*MAX_HUNTERS,
                   PROT_READ|PROT_WRITE, MAP_SHARED, hfd,0);
    dungeons= mmap(NULL, sizeof(dungeon_t)*MAX_DUNGEONS,
                   PROT_READ|PROT_WRITE, MAP_SHARED, dfd,0);
    hsem = sem_open(HUNTER_SEM,0);
    dsem = sem_open(DUNGEON_SEM,0);

    int choice; int mykey;
    printf("1) Register\n2) Login\nChoice: "); scanf("%d",&choice);
    if (choice==1) {
        mykey = register_hunter();
        if (mykey<0) return 1;
    }
    me = NULL;
    while (!me) me = login_hunter();

    while (1) {
        printf("\n[%s | Lvl %d] Menu:\n", me->name, me->level);
        printf("1) View Dungeons\n2) Raid Dungeon\n3) Battle Hunter\n");
        printf("4) Toggle Notifications (%s)\n5) Logout\nChoice: ",
               me->notify?"ON":"OFF");
        scanf("%d",&choice);
        switch (choice) {
            case 1: view_dungeons(); break;
            case 2: raid_dungeon(); break;
            case 3: battle_hunter(); break;
            case 4:
                me->notify = !me->notify;
                if (me->notify) {
                    pthread_create(&notif_thread,NULL,notif_func,NULL);
                } else {
                    // notif thread akan berhenti sendiri
                }
                break;
            case 5:
                printf("Goodbye!\n");
                return 0;
            default: printf("Invalid.\n");
        }
    }

    return 0;
}
