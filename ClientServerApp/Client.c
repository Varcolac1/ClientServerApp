#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void *receive_messages(void *arg) {
    int socket = *(int *)arg;
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_read = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            printf("Sunucudan bağlantı kesildi.\n");
            break;
        }
        buffer[bytes_read] = '\0'; // Null-terminate the string
        printf("Sunucudan cevap: %s\n", buffer);
    }
    return NULL;
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    pthread_t tid;

    // Soket oluştur
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Sunucu adresini tanımla
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Geçersiz adres / Adres desteklenmiyor");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Sunucuya bağlan
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bağlantı başarısız");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Sunucuya bağlanıldı.\n");

    // Mesaj alma işlevini başlat
    if (pthread_create(&tid, NULL, receive_messages, &client_socket) != 0) {
        perror("Thread oluşturulamadı");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Kullanıcıdan kelime al ve sunucuya gönder
    while (1) {
        printf("İngilizce kelime gir (çıkmak için 'exit'): ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0'; // Yeni satırı kaldır

        if (strcmp(buffer, "exit") == 0) {
            printf("Çıkılıyor...\n");
            break;
        }

        send(client_socket, buffer, strlen(buffer), 0);
    }

    close(client_socket);
    pthread_cancel(tid);
    pthread_join(tid, NULL);
    printf("Bağlantı kapatıldı.\n");
    return 0;
}