#include <arpa/inet.h>  // Inclut l'en-tête pour les fonctions liées à inet (Internet).
#include <netdb.h>      // Inclut l'en-tête pour les fonctions liées aux informations réseau.
#include <netinet/in.h> // Inclut l'en-tête pour les structures et constantes Internet.
#include <stdio.h>      // Inclut l'en-tête standard pour les entrées/sorties.
#include <stdlib.h>     // Inclut l'en-tête standard pour les fonctions de gestion de la mémoire.
#include <string.h>     // Inclut l'en-tête standard pour les fonctions de manipulation de chaînes de caractères.
#include <sys/socket.h> // Inclut l'en-tête pour les fonctions liées aux sockets.
#include <unistd.h>     // Inclut l'en-tête pour les fonctions liées aux appels système.

#include "common.h"     // Inclut l'en-tête personnalisé "common.h".

// Définition de la fonction echo_server prenant un descripteur de socket en argument.
void echo_server(int sockfd) {
    char buff[MSG_LEN]; // Déclare un tableau de caractères pour stocker les messages.

    while (1) { // Boucle infinie pour la communication.
        // Cleaning memory
        memset(buff, 0, MSG_LEN); // Efface la mémoire tampon des messages.

        // Receiving message
        if (recv(sockfd, buff, MSG_LEN, 0) <= 0) { // Reçoit un message du client.
            break; // Sort de la boucle si la réception échoue.
        }
        printf("Received: %s", buff); // Affiche le message reçu.

        // Sending message (ECHO)
        if (send(sockfd, buff, strlen(buff), 0) <= 0) { // Envoie le message sur le socket.
            break; // Sort de la boucle si l'envoi échoue.
        }
        printf("Message sent!\n"); // Affiche un message de confirmation d'envoi.
    }
}

// Fonction pour gérer la liaison du serveur.
int handle_bind(char *server_port) {
    struct addrinfo hints, *result, *rp;
    int sfd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // Indique que l'on peut utiliser IPv4 ou IPv6.
    hints.ai_socktype = SOCK_STREAM; // Indique que l'on utilise un socket de type TCP.
    hints.ai_flags = AI_PASSIVE; // Indique que l'on écoute sur toutes les interfaces.

    // Obtient les informations locales pour la liaison du serveur.
    if (getaddrinfo(NULL, server_port, &hints, &result) != 0) {
        perror("getaddrinfo()"); // Affiche un message d'erreur en cas d'échec.
        exit(EXIT_FAILURE); // Quitte le programme en cas d'erreur.
    }

    // Parcourt la liste des résultats obtenus à partir de getaddrinfo.
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) {
            continue; // Continue la boucle si la création du socket échoue.
        }
        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break; // Établit la liaison du serveur et sort de la boucle.
        }
        close(sfd); // Ferme le socket si la liaison échoue.
    }

    // Vérifie si rp est toujours NULL (liaison échouée).
    if (rp == NULL) {
        fprintf(stderr, "Could not bind\n"); // Affiche un message d'erreur.
        exit(EXIT_FAILURE); // Quitte le programme en cas d'échec de la liaison.
    }

    freeaddrinfo(result); // Libère la mémoire utilisée par les informations obtenues.

    return sfd; // Retourne le descripteur de socket lié au serveur.
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_name> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_port = argv[1];

    struct sockaddr cli;
    int sfd, connfd;
    socklen_t len;
    sfd = handle_bind(server_port); // Établit la liaison du serveur.
    if ((listen(sfd, SOMAXCONN)) != 0) {
        perror("listen()\n"); // Affiche un message d'erreur en cas d'échec.
        exit(EXIT_FAILURE); // Quitte le programme en cas d'erreur.
    }
    len = sizeof(cli);
    if ((connfd = accept(sfd, (struct sockaddr*) &cli, &len)) < 0) {
        perror("accept()\n"); // Affiche un message d'erreur en cas d'échec.
        exit(EXIT_FAILURE); // Quitte le programme en cas d'erreur.
    }
    echo_server(connfd); // Lance la fonction de communication serveur-client.
    close(sfd); // Ferme le socket du serveur.
    return EXIT_SUCCESS;
}
