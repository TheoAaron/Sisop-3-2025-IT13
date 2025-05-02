#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

#define MAX_ORDER 100
#define CSV_FILE "delivery_order.csv"

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

void download_csv()
{
    if (access("delivery_order.csv", F_OK) != -1)
    {
        printf("⬇️ File delivery_order.csv sudah ada, skip download.\n");
        return;
    }

    printf("📦 Mengunduh delivery_order.csv...\n");
    const char *command =
        "wget --no-check-certificate 'https://docs.google.com/uc?export=download&id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9' "
        "-O delivery_order.csv";

    int status = system(command);
    if (status != 0)
    {
        fprintf(stderr, "❌ Gagal mengunduh file CSV.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("✅ Berhasil mengunduh delivery_order.csv.\n");
    }
}

void muat_csv(SharedData *shared)
{
    FILE *fp = fopen(CSV_FILE, "r");
    if (!fp)
    {
        perror("❌ Gagal membuka file CSV");
        exit(EXIT_FAILURE);
    }

    char baris[200];
    int is_header = 1;

    while (fgets(baris, sizeof(baris), fp))
    {
        if (is_header)
        {
            is_header = 0;
            continue;
        }

        if (sscanf(baris, "%[^,],%[^,],%s", shared->data[shared->jumlah_pesanan].nama,
                   shared->data[shared->jumlah_pesanan].alamat,
                   shared->data[shared->jumlah_pesanan].tipe) == 3)
        {
            shared->data[shared->jumlah_pesanan].dikirim = 0;
            strcpy(shared->data[shared->jumlah_pesanan].agen, "-");
            shared->jumlah_pesanan++;
        }
    }
    fclose(fp);
}

void tulis_log(const char *user_agent, const Pesanan *order)
{
    FILE *log = fopen("delivery.log", "a");
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char waktu[100];
    strftime(waktu, sizeof(waktu), "%d/%m/%Y %H:%M:%S", t);
    fprintf(log, "[%s] [AGENT %s] Reguler package delivered to %s in %s\n",
            waktu, user_agent, order->nama, order->alamat);
    fclose(log);
}

void deliver_reguler(SharedData *shared, const char *nama, const char *user_agent)
{
    pthread_mutex_lock(&shared->mutex);
    int found = 0;
    for (int i = 0; i < shared->jumlah_pesanan; i++)
    {
        if (strcmp(shared->data[i].nama, nama) == 0 &&
            strcmp(shared->data[i].tipe, "Reguler") == 0 &&
            shared->data[i].dikirim == 0)
        {

            shared->data[i].dikirim = 1;
            strcpy(shared->data[i].agen, user_agent);
            tulis_log(user_agent, &shared->data[i]);
            printf("Paket berhasil dikirim oleh AGENT %s.\n", user_agent);
            found = 1;
            break;
        }
    }
    if (!found)
    {
        printf("❌ Pesanan tidak ditemukan atau sudah dikirim.\n");
    }
    pthread_mutex_unlock(&shared->mutex);
}

void cek_status(SharedData *shared, const char *nama)
{
    pthread_mutex_lock(&shared->mutex);
    for (int i = 0; i < shared->jumlah_pesanan; i++)
    {
        if (strcmp(shared->data[i].nama, nama) == 0)
        {
            if (shared->data[i].dikirim)
                printf("Status for %s: Delivered by %s\n", nama, shared->data[i].agen);
            else
                printf("Status for %s: Pending\n", nama);
            pthread_mutex_unlock(&shared->mutex);
            return;
        }
    }
    printf("❌ Pesanan tidak ditemukan.\n");
    pthread_mutex_unlock(&shared->mutex);
}

void list_pesanan(SharedData *shared)
{
    pthread_mutex_lock(&shared->mutex);
    for (int i = 0; i < shared->jumlah_pesanan; i++)
    {
        printf("%s - %s\n", shared->data[i].nama,
               shared->data[i].dikirim ? shared->data[i].agen : "Pending");
    }
    pthread_mutex_unlock(&shared->mutex);
}

int main(int argc, char *argv[])
{
    download_csv();

    key_t key = 1234;
    int shmid = shmget(key, sizeof(SharedData), IPC_CREAT | 0666);
    SharedData *shared = shmat(shmid, NULL, 0);

    if (shared->jumlah_pesanan == 0)
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&shared->mutex, &attr);
        muat_csv(shared);
    }

    if (argc == 3 && strcmp(argv[1], "-deliver") == 0)
    {
        char *user = getenv("USER");
        deliver_reguler(shared, argv[2], user ? user : "USER");
    }
    else if (argc == 3 && strcmp(argv[1], "-status") == 0)
    {
        cek_status(shared, argv[2]);
    }
    else if (argc == 2 && strcmp(argv[1], "-list") == 0)
    {
        list_pesanan(shared);
    }
    else
    {
        printf("Usage:\n");
        printf("  ./dispatcher -deliver [Nama]\n");
        printf("  ./dispatcher -status [Nama]\n");
        printf("  ./dispatcher -list\n");
    }

    return 0;
}
