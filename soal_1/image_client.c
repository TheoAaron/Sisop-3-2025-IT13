#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 50000

void send_command(int sock, const char *command) {
    send(sock, command, strlen(command), 0);
    
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        perror("recv failed");
        return;
    }
    
    buffer[bytes_received] = '\0';
    printf("Server response: %s\n", buffer);
}

void download_file(int sock, const char *filename) {
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "DOWNLOAD:%s", filename);
    send(sock, command, strlen(command), 0);
    
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        perror("recv failed");
        return;
    }
    
    if (strncmp(buffer, "FILE_NOT_FOUND", 14) == 0) {
        printf("File not found on server.\n");
        return;
    }
    
    char filepath[BUFFER_SIZE];
    snprintf(filepath, sizeof(filepath), "client/%s", filename);

    FILE *file = fopen(filepath, "wb");
    if (file == NULL) {
        perror("fopen failed");
        return;
    }
    
    fwrite(buffer, 1, bytes_received, file);
    fclose(file);
    
    printf("File downloaded successfully: %s\n", filename);
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("invalid address");
        exit(EXIT_FAILURE);
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server.\n");
    
    while (1) {
        printf("\nMenu:\n");
        printf("1. Decrypt and save file\n");
        printf("2. Download file\n");
        printf("3. Exit\n");
        printf("Enter choice: ");
        
        int choice;
        scanf("%d", &choice);
        getchar();
        
        switch (choice) {
            case 1: {
                printf("Enter hex encoded file text to decrypt: ");
                char hex_text[BUFFER_SIZE];
                fgets(hex_text, BUFFER_SIZE, stdin);
                hex_text[strcspn(hex_text, "\n")] = '\0';
                
                char command[BUFFER_SIZE];
                snprintf(command, sizeof(command), "DECRYPT:%s", hex_text);
                send_command(sock, command);
                break;
            }
            case 2: {
                printf("Enter filename to download (e.g., 123.jpeg): ");
                char filename[BUFFER_SIZE];
                fgets(filename, BUFFER_SIZE, stdin);
                filename[strcspn(filename, "\n")] = '\0';
                
                download_file(sock, filename);
                break;
            }
            case 3:
                send_command(sock, "exit");
                close(sock);
                exit(EXIT_SUCCESS);
            default:
                printf("Invalid choice.\n");
        }
    }
    
    return 0;
}
