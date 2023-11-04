#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>

struct Client {
    int sockfd;
    char* nickname;
    struct in_addr addr;
    unsigned short int port;
    time_t connectionTime;
    struct Client* next;
};

struct Client* head = NULL;  // Pointeur vers le début de la liste

// Fonction pour ajouter un client en bout de chaîne
void addClient(int sockfd, const char* nickname, struct in_addr addr, unsigned short int port, time_t connectionTime) {
    struct Client* newClient = (struct Client*)malloc(sizeof(struct Client));
    newClient->sockfd = sockfd;
    newClient->nickname = strdup(nickname);
    newClient->addr = addr;
    newClient->port = port;
    newClient->connectionTime = connectionTime;
    newClient->next = NULL;

    if (head == NULL) {
        head = newClient;  // Si la liste est vide, le nouveau client devient la tête
    } else {
        struct Client* current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newClient;
    }
}

// Fonction pour supprimer un client de la chaîne par sockfd
void removeClient(int sockfd) {
    if (head == NULL) {
        return;  // Rien à faire si la liste est vide
    }

    if (head->sockfd == sockfd) {
        struct Client* temp = head;
        head = head->next;
        free(temp->nickname);
        free(temp);
        return;
    }

    struct Client* current = head;
    while (current->next != NULL) {
        if (current->next->sockfd == sockfd) {
            struct Client* temp = current->next;
            current->next = current->next->next;
            free(temp->nickname);
            free(temp);
            return;
        }
        current = current->next;
    }
}

// Fonction pour afficher la liste des clients
void displayClients() {
    struct Client* current = head;
    printf("Liste des clients:\n");
    while (current != NULL) {
        printf("sockfd: %d, nickname: %s, port: %d, connectionTime: %ld\n",
               current->sockfd, current->nickname, current->port, current->connectionTime);
        current = current->next;
    }
}

// Fonction pour libérer la mémoire de la liste à la fin
void freeClients() {
    while (head != NULL) {
        struct Client* temp = head;
        head = head->next;
        free(temp->nickname);
        free(temp);
    }
}

int main() {
    // Exemple d'utilisation des fonctions
    addClient(1, "Client1", (struct in_addr){.s_addr = 0x01010101}, 1234, time(NULL));
    addClient(2, "Client2", (struct in_addr){.s_addr = 0x02020202}, 5678, time(NULL));
    addClient(3, "Client3", (struct in_addr){.s_addr = 0x03030303}, 9876, time(NULL));

    displayClients();

    removeClient(2);

    displayClients();

    removeClient(1);

    displayClients();

    freeClients();

    return 0;
}
