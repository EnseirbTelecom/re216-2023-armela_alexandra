#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <poll.h>

#include "common.h"

#define MAX_CLIENTS 10

struct Client {
    int sockfd;
    struct sockaddr cli;
    struct Client * next;
};



// Fonction pour créer et lier le socket serveur.
int create_and_bind(char *server_port) {
    struct addrinfo hints, *result, *rp;
    int sfd;  // Le descripteur de socket du serveur.

    // Configuration des paramètres pour obtenir des informations sur le serveur.
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Obtention des informations locales pour la liaison du serveur.
    if (getaddrinfo(NULL, server_port, &hints, &result) != 0) {
        perror("getaddrinfo()");  // Affiche une erreur si getaddrinfo échoue.
        exit(EXIT_FAILURE);
    }

    // Création du socket et liaison avec l'adresse et le port.
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) {
            continue;  // Continue la boucle si la création du socket échoue.
        }
        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;  // Établit la liaison du serveur et sort de la boucle.
        }
        close(sfd);  // Ferme le socket si la liaison échoue.
    }

    if (rp == NULL) {
        fprintf(stderr, "Could not bind\n");  // Affiche un message d'erreur en cas d'échec de la liaison.
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);  // Libère la mémoire utilisée par les informations obtenues.

    return sfd;  // Retourne le descripteur de socket du serveur.
}

void handle_new_client(int sfd, struct pollfd *fds, struct Client **clients_head, int *num_clients) {
    struct sockaddr cli;             // Structure pour stocker les informations du client.
    socklen_t len = sizeof(cli);     // Taille de la structure pour accept().
    int connfd = accept(sfd, (struct sockaddr *) &cli, &len);  // Accepte une nouvelle connexion.

    if (connfd < 0) {
        perror("accept()");  // Affiche une erreur si accept échoue.
        return;
    }

    if (*num_clients < MAX_CLIENTS) {
        // Ajoute le descripteur de socket du client à la structure pollfd.
        fds[*num_clients + 1].fd = connfd;
        fds[*num_clients + 1].events = POLLIN;  // Événements à surveiller pour le client.
        fds[*num_clients + 1].revents=0;
        struct Client *new_client = (struct Client *)malloc(sizeof(struct Client));
        new_client->sockfd = connfd;
        new_client->cli = cli;
        new_client->next = NULL;
        
        if (*clients_head == NULL) {
            *clients_head = new_client;
        } else {
            struct Client *current = *clients_head;
            while (current->next != NULL) {
                current = current->next;
            }
            current->next = new_client;
        }

        (*num_clients)++;  // Incrémente le nombre de clients.
        printf("New client connected. Total clients: %d\n", *num_clients);
    } else {
        printf("Maximum number of clients reached. Connection rejected.\n");
        close(connfd);  // Ferme la connexion si le nombre maximum de clients est atteint.
    }
}

/*
// Fonction pour gérer l'arrivée d'un nouveau client.
void handle_new_client(int sfd, struct pollfd *fds, struct Client *clients, int *num_clients) {
    struct sockaddr cli;             // Structure pour stocker les informations du client.
    socklen_t len = sizeof(cli);     // Taille de la structure pour accept().
    int connfd = accept(sfd, (struct sockaddr *) &cli, &len);  // Accepte une nouvelle connexion.

    if (connfd < 0) {
        perror("accept()");  // Affiche une erreur si accept échoue.
        return;
    }

    if (*num_clients < MAX_CLIENTS) {
        // Ajoute le descripteur de socket du client à la structure pollfd.
        fds[*num_clients + 1].fd = connfd;
        fds[*num_clients + 1].events = POLLIN;  // Événements à surveiller pour le client.
        clients->sockfd = connfd;  // Stocke le descripteur de socket du client.
        (*num_clients)++;  // Incrémente le nombre de clients.
        printf("New client connected. Total clients: %d\n", *num_clients);
    } else {
        printf("Maximum number of clients reached. Connection rejected.\n");
        close(connfd);  // Ferme la connexion si le nombre maximum de clients est atteint.
    }
    clients->cli=cli;
}
*/
void remove_client(int sockfd,struct Client * clients_head) {
    if (clients_head == NULL) {
        return;  // La liste est vide, pas de client à supprimer.
    }

    struct Client *current = clients_head;
    struct Client *previous = NULL;

    while (current != NULL) {
        if (current->sockfd == sockfd) {
            if (previous == NULL) {
                clients_head = current->next;
            } else {
                previous->next = current->next;
            }

            close(current->sockfd);  // Ferme la connexion du client.
            
            break;
        }

        previous = current;
        current = current->next;
    }
    if (current != NULL)
    {
        free(current);  // Libère la mémoire allouée pour le client.
    }
    
    
}


// Fonction pour gérer les données d'un client existant.
void handle_client_data(int sockfd, struct Client *clients,struct Client *clients_head, int *num_clients, int idx) {
    char buff[MSG_LEN];
    memset(buff, 0, MSG_LEN);

    if (recv(sockfd, buff, MSG_LEN, 0) <= 0) {
        printf("Client disconnected. Total clients: %d\n", *num_clients - 1);

        // Supprime le client déconnecté de la liste des clients.
        remove_client(sockfd,clients_head);
        (*num_clients)--;

        close(sockfd);  // Ferme la connexion avec le client déconnecté.

    } else {
        printf("Received from client %d: %s", sockfd , buff);

        if (strcmp(buff, "/quit\n") == 0) {
            printf("Client requested to close the connection. Closing...\n");
            // Envoyer la chaîne de caractères reçue uniquement au client qui l'a envoyée.
            
            int ret=send(sockfd, buff, strlen(buff), 0); 
            if (ret <= 0) {
                perror("send()");
                }
            
            close(sockfd);  // Ferme la connexion du client.
            printf("Client disconnected. Total clients: %d\n", *num_clients - 1);

            // Supprime le client déconnecté de la liste des clients.
            remove_client(sockfd,clients_head);

            (*num_clients)--;

        } 
        // Envoyer la chaîne de caractères reçue uniquement au client qui l'a envoyée.
       
        else{ 
            if (send(sockfd, buff, strlen(buff), 0) <= 0) {
                perror("send()");
            } else {
                printf("%d",sockfd);
                printf("Message sent to client %d!\n", sockfd );
            }
        }
    }
        
    
}




int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_port = argv[1];
    int sfd = create_and_bind(server_port); 

    if (listen(sfd, SOMAXCONN) != 0) {
        perror("listen()");
        exit(EXIT_FAILURE);
    }

    struct pollfd fds[MAX_CLIENTS + 1];  // Tableau pour surveiller les descripteurs de socket.
    struct Client *clients_head = NULL;   // Tableau pour stocker les informations sur les clients.
    int num_clients = 0;  

    fds[0].fd = sfd;
    fds[0].events = POLLIN;

    while (1) {
        int ret = poll(fds, num_clients + 1, -1);  // Attend des événements sur les descripteurs.

        if (ret == -1) {
            perror("poll()");
            exit(EXIT_FAILURE);
        }

        // Vérifie si le socket serveur est prêt à accepter de nouvelles connexions.
        if (fds[0].revents & POLLIN) {
            
            handle_new_client(sfd, fds, &clients_head, &num_clients); 
            
        }

        // Parcourt les clients existants pour gérer les données entrantes.
        for (int i = 0; i < num_clients; i++) {
            struct Client *current = clients_head;
            if (fds[i + 1].revents & POLLIN) {     
                while (current->sockfd != fds[i + 1].fd) {
                    current = current->next;
                    
                }

            }
                
            handle_client_data(current->sockfd, current,clients_head, &num_clients, i); // Gère les données d'un client existant.
        }

    }
    close(sfd); 
    return EXIT_SUCCESS;
} 