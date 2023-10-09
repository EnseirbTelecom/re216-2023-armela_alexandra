#include <arpa/inet.h>  // Inclut l'en-tête pour les fonctions liées à inet (Internet).
#include <netdb.h>      // Inclut l'en-tête pour les fonctions liées aux informations réseau.
#include <netinet/in.h> // Inclut l'en-tête pour les structures et constantes Internet.
#include <stdio.h>      // Inclut l'en-tête standard pour les entrées/sorties.
#include <stdlib.h>     // Inclut l'en-tête standard pour les fonctions de gestion de la mémoire.
#include <string.h>     // Inclut l'en-tête standard pour les fonctions de manipulation de chaînes de caractères.
#include <sys/socket.h> // Inclut l'en-tête pour les fonctions liées aux sockets.
#include <unistd.h>     // Inclut l'en-tête pour les fonctions liées aux appels système.

#include "common.h"     // Inclut l'en-tête personnalisé "common.h".

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

// Fonction pour gérer la connexion au serveur.
int handle_connect(char *server_name,char *server_port) {
    struct addrinfo hints, *result, *rp;
    int sfd;

    // Initialise la structure hints à zéro et configure les paramètres de la connexion.
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC; // Indique que l'on peut utiliser IPv4 ou IPv6.
    hints.ai_socktype = SOCK_STREAM; // Indique que l'on utilise un socket de type TCP.

    // Obtient les informations sur l'hôte distant (adresse IP et port).
    if (getaddrinfo(server_name, server_port, &hints, &result) != 0) {
        perror("getaddrinfo()"); // Affiche un message d'erreur en cas d'échec.
        exit(EXIT_FAILURE); // Quitte le programme en cas d'erreur.
    }

    // Parcourt la liste des résultats obtenus à partir de getaddrinfo.
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1) {
            continue; // Continue la boucle si la création du socket échoue.
        }
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            break; // Établit la connexion avec le serveur et sort de la boucle.
        }
        close(sfd); // Ferme le socket si la connexion échoue.
    }

    // Vérifie si rp est toujours NULL (connexion échouée).
    if (rp == NULL) {
        fprintf(stderr, "Could not connect\n"); // Affiche un message d'erreur.
        exit(EXIT_FAILURE); // Quitte le programme en cas d'échec de la connexion.
    }

    freeaddrinfo(result); // Libère la mémoire utilisée par les informations obtenues.

    return sfd; // Retourne le descripteur de socket connecté au serveur.
}


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
