#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>

#define PORT 8080
#define BUFFER_SIZE 50000
#define DATABASE_DIR "database"

void make_log(const char* role, const char* action, const char* filename) {
    FILE *log_file;
    time_t now;
    struct tm *timeinfo;
    char timestamp[20];

    log_file = fopen("server/server.log", "a");
    if (log_file == NULL) {
        perror("Gagal membuka file log");
        return;
    }

    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    fprintf(log_file, "[%s][%s]: [%s]", role, timestamp, action);
    
    if (filename != NULL && strlen(filename) > 0) {
        fprintf(log_file, " [%s]", filename);
    }
    
    fprintf(log_file, "\n");
    fclose(log_file);
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    umask(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void reverse_string(char *str) {
    int len = strlen(str);
    for (int i = 0; i < len / 2; i++) {
        char temp = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = temp;
    }
}

int hex_decode(const char *hex, char *output, int max_output_len) {
    int len = strlen(hex);
    
    if (len % 2 != 0 || len/2 > max_output_len) {
        return -1; 
    }
    for (int i = 0; i < len; i += 2) {
        if (sscanf(hex + i, "%2hhx", &output[i/2]) != 1) {
            return -1; 
        }
    }
    return len/2; 
}

int decrypt_file(const char *input, char *output, int max_output_len) {
    char reversed[BUFFER_SIZE];
    if (strlen(input) >= BUFFER_SIZE) return -1;
    strcpy(reversed, input);
    reverse_string(reversed);
    
    return hex_decode(reversed, output, max_output_len);
}

void save_to_database(const char *data, int data_len, const char *filename) {
    char path[256];
    snprintf(path, sizeof(path), "server/database/%s", filename);
    
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        perror("fopen failed");
        return;
    }
    
    fwrite(data, 1, data_len, file);
    fclose(file);
}

char* read_text_file(const char *filename) {
    char fullpath[256];
    snprintf(fullpath, sizeof(fullpath), "client/secrets/%s", filename);

    FILE *file = fopen(fullpath, "rb");
    if (!file) {
        perror("fopen failed");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(file_size + 1);
    if (!content) {
        perror("malloc failed");
        fclose(file);
        return NULL;
    }

    fread(content, 1, file_size, file);
    content[file_size] = '\0';

    fclose(file);
    return content;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0))) {
        if (bytes_received < 0) {
            perror("recv failed");
            break;
        }
        
        buffer[bytes_received] = '\0';
        
        if (strncmp(buffer, "DECRYPT:", 8) == 0) {
            char *txt_filename = buffer + 8;
            txt_filename[strcspn(txt_filename, "\n")] = '\0';
            
            char *hex_content = read_text_file(txt_filename);
            if (!hex_content) {
                send(client_socket, "ERROR: Failed to read file", 26, 0);
                continue;
            }

            char decrypted_data[BUFFER_SIZE * 2];
            int decrypted_len = decrypt_file(hex_content, decrypted_data, sizeof(decrypted_data));
            
            if (decrypted_len <= 0) {
                send(client_socket, "ERROR: Decryption failed", 24, 0);
                free(hex_content);
                continue;
            }

            time_t now = time(NULL);
            char filename[256];
            snprintf(filename, sizeof(filename), "%ld.jpeg", now);
            
            save_to_database(decrypted_data, decrypted_len, filename);
            
            send(client_socket, filename, strlen(filename), 0);
            make_log("Client", "DECRYPT", "Text data");
            make_log("Server", "SAVE", filename);
        } else if (strncmp(buffer, "DOWNLOAD:", 9) == 0) {
            char *filename = buffer + 9;
            char path[256];
            snprintf(path, sizeof(path), "server/database/%s", filename);
            
            filename[strcspn(filename, "\n")] = '\0';

            FILE *file = fopen(path, "rb");
            if (file == NULL) {
                send(client_socket, "FILE_NOT_FOUND", 14, 0);
                continue;
            }
            
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            fseek(file, 0, SEEK_SET);

            if (file_size <= 0)
            {
                send(client_socket, "ERROR:EMPTY_FILE", 16, 0);
                fclose(file);
                continue;
            }
            
            char header[64];
            snprintf(header, sizeof(header), "FILE_SIZE:%ld", file_size);
            //send(client_socket, header, strlen(header), 0);
            
            char buffer[4096];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                if (send(client_socket, buffer, bytes_read, 0) < 0) {
                    perror("send failed");
                    break;
                }
            }
            
            fclose(file);
            printf("File %s sent successfully\n", filename);
            make_log("Client", "DOWNLOAD", filename);
            make_log("Server", "UPLOAD", filename);
        } 
        else if (strncmp(buffer, "exit") == 0) {
            make_log("Client", "EXIT", NULL);
            close(client_socket);
            return;  
        }else {
            send(client_socket, "INVALID_COMMAND", 15, 0);
        }
    }
    
    close(client_socket);
}

int main() {
    daemonize();

    const char *parent_dir = "server";
    const char *child_dir = "server/database";

    mkdir(parent_dir, 0777);
    mkdir(child_dir, 0777);
    
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(client_addr);
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_socket, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_size);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }
        
        handle_client(client_socket);
    }
    
    close(server_socket);
    return 0;
}
