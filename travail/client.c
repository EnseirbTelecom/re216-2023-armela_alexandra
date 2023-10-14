#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"

#define MAX_EVENTS 2

struct pollfd poll_fds[MAX_EVENTS];

void echo_client(int sockfd) {
    char buff[MSG_LEN];
    int n;

    while (1) {
        int poll_count = poll(poll_fds, MAX_EVENTS, -1);

        if (poll_count < 0) {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        // If there is data available from the keyboard (stdin).
        if (poll_fds[0].revents & POLLIN) {
            memset(buff, 0, MSG_LEN);
            n = 0;
            while ((buff[n++] = getchar()) != '\n') {}

            // Send the message to the server.
            if (send(sockfd, buff, strlen(buff), 0) <= 0) {
                perror("send");
                break;
            }
            printf("Message sent: %s\n", buff);
        }

        // If data is available from the server.
        if (poll_fds[1].revents & POLLIN) {
            memset(buff, 0, MSG_LEN);

            // Receive data from the server.
            if (recv(sockfd, buff, MSG_LEN, 0) <= 0) {
                perror("recv");
                break;
            }

            if (strcmp(buff, "/quit\n") == 0) {
                printf("Server accepted the request to close the connection. Closing...\n");
                close(poll_fds[1].fd);  // Ferme la connexion du client.
                exit(EXIT_SUCCESS);  // Arrête le programme client.
            }
            printf("Received from server: %s\n", buff);
            
        }
    }
}




int handle_connect(char *server_name, char *server_port) {
    struct addrinfo hints, *result, *rp;
    int sfd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server_name, server_port, &hints, &result) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) {
            continue;
        }
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;
        }
        close(sfd);
    }

    if (rp == NULL) {
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    return sfd;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_name> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_name = argv[1];
    char *server_port = argv[2];

    int sockfd = handle_connect(server_name, server_port);

    poll_fds[0].fd = fileno(stdin);
    poll_fds[0].events = POLLIN;
    poll_fds[1].fd = sockfd;
    poll_fds[1].events = POLLIN;

    echo_client(sockfd);

    close(sockfd);

    return EXIT_SUCCESS;
}


/*
// Définition de la fonction echo_client prenant un descripteur de socket en argument.
void echo_client(int sockfd) {
    char buff[MSG_LEN]; // Déclare un tableau de caractères pour stocker les messages.
    int n;

    while (1) { // Boucle infinie pour la communication.
        // Cleaning memory
        memset(buff, 0, MSG_LEN); // Efface la mémoire tampon des messages.

        // Getting message from client
        printf("Message: "); // Affiche un message à l'utilisateur.
        n = 0;
        while ((buff[n++] = getchar()) != '\n') {} // Lit les caractères entrés par l'utilisateur jusqu'à '\n'.

        // Sending message (ECHO)
        if (send(sockfd, buff, strlen(buff), 0) <= 0) { // Envoie le message sur le socket.
            break; // Sort de la boucle si l'envoi échoue.
        }
        printf("Message sent!\n"); // Affiche un message de confirmation d'envoi.

        // Cleaning memory
        memset(buff, 0, MSG_LEN); // Efface à nouveau la mémoire tampon des messages.

        // Receiving message
        if (recv(sockfd, buff, MSG_LEN, 0) <= 0) { // Reçoit un message du serveur.
            break; // Sort de la boucle si la réception échoue.
        }
        printf("Received: %s", buff); // Affiche le message reçu.
    }
}
*/




/*
int main(int argc, char *argv[]) {
	if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_name> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_name = argv[1];
    char *server_port = argv[2];

    int sfd;

    sfd = handle_connect(server_name,server_port); // Établit la connexion au serveur.
    echo_client(sfd); // Lance la fonction de communication client-serveur.
    close(sfd); // Ferme le socket.
    return EXIT_SUCCESS;
}

*/