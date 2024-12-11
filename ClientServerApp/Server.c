#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Basit İngilizce-Türkçe sözlük
typedef struct {
    char english[BUFFER_SIZE];
    char turkish[BUFFER_SIZE];
} DictionaryEntry;

DictionaryEntry dictionary[] = {
    {"hello", "merhaba"},
    {"world", "dünya"},
    {"computer", "bilgisayar"},
    {"science", "bilim"},
    {"network", "ağ"},
    {"socket", "yuva"},
    {"programming", "programlama"},
    {"language", "dil"},
    {"goodbye", "hoşça kal"},
    {"apple", "elma"}
};
int dictionary_size = sizeof(dictionary) / sizeof(dictionary[0]);

int client_sockets[MAX_CLIENTS];
int client_count = 0;

pthread_mutex_t client_lock;

void translate_and_respond(int client_socket, char *word) {
    char response[BUFFER_SIZE] = "Bu kelime sözlükte yok.";

    // Kelimeyi sözlükte arayın
    for (int i = 0; i < dictionary_size; i++) {
        if (strcmp(word, dictionary[i].english) == 0) {
            snprintf(response, sizeof(response), "Türkçe karşılığı: %s", dictionary[i].turkish);
            break;
        }
    }

    // Sonucu istemciye gönder
    send(client_socket, response, strlen(response), 0);
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        printf("İstemciden gelen kelime: %s\n", buffer);

        // Kelimeyi çevir ve cevap gönder
        translate_and_respond(client_socket, buffer);
    }

    // İstemciyi bağlantı listesinden çıkar
    pthread_mutex_lock(&client_lock);
    for (int i = 0; i < client_count; i++) {
        if (client_sockets[i] == client_socket) {
            client_sockets[i] = client_sockets[--client_count];
            break;
        }
    }
    pthread_mutex_unlock(&client_lock);

    close(client_socket);
    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;

    pthread_mutex_init(&client_lock, NULL);

    // Sunucu soketi oluştur
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Soketi bağla
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Bağlantıları dinle
    listen(server_socket, MAX_CLIENTS);
    printf("Sunucu %d portunda dinliyor...\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Bağlantı kabul edilemedi");
            continue;
        }

        pthread_mutex_lock(&client_lock);
        if (client_count < MAX_CLIENTS) {
            client_sockets[client_count++] = client_socket;
            pthread_create(&tid, NULL, handle_client, &client_socket);
        } else {
            printf("Maksimum istemci sayısına ulaşıldı. Bağlantı reddedildi.\n");
            close(client_socket);
        }
        pthread_mutex_unlock(&client_lock);
    }

    close(server_socket);
    pthread_mutex_destroy(&client_lock);
    return 0;
}