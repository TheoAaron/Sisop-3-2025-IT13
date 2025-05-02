#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_ORDER 100

typedef struct
{
    char nama[50];
    char alamat[100];
    char tipe[10];
    int dikirim;
    char agen[20];
} Pesanan;

typedef struct
{
    Pesanan data[MAX_ORDER];
    int jumlah_pesanan;
    pthread_mutex_t mutex;
} SharedData;

void tulis_log(const char *agent, const Pesanan *order)
{
    FILE *log = fopen("delivery.log", "a");
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char waktu[100];
    strftime(waktu, sizeof(waktu), "%d/%m/%Y %H:%M:%S", t);
    fprintf(log, "[%s] [%s] Express package delivered to %s in %s\n",
            waktu, agent, order->nama, order->alamat);
    fclose(log);
}

void *agent_thread(void *arg)
{
    char *agent_name = (char *)arg;
    key_t key = 1234;
    int shmid = shmget(key, sizeof(SharedData), 0666);
    SharedData *shared = shmat(shmid, NULL, 0);

    while (1)
    {
        pthread_mutex_lock(&shared->mutex);
        for (int i = 0; i < shared->jumlah_pesanan; i++)
        {
            if (strcmp(shared->data[i].tipe, "Express") == 0 && shared->data[i].dikirim == 0)
            {
                shared->data[i].dikirim = 1;
                strcpy(shared->data[i].agen, agent_name);
                tulis_log(agent_name, &shared->data[i]);
                break;
            }
        }
        pthread_mutex_unlock(&shared->mutex);
        sleep(1);
    }

    return NULL;
}

int main()
{
    pthread_t a, b, c;
    pthread_create(&a, NULL, agent_thread, "AGENT A");
    pthread_create(&b, NULL, agent_thread, "AGENT B");
    pthread_create(&c, NULL, agent_thread, "AGENT C");

    pthread_join(a, NULL);
    pthread_join(b, NULL);
    pthread_join(c, NULL);
    return 0;
}
