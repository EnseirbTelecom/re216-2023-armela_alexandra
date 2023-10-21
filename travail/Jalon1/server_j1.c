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

#include "common.h"

struct Client {
    int sockfd;
    struct in_addr addr;
	unsigned short int port;
    struct Client * next;
};


int echo_server(int* num_clients,struct pollfd * fds,int i) {
	char buff[MSG_LEN];
	int sockfd=fds[i].fd;
	
	// Cleaning memory
	memset(buff, 0, MSG_LEN);
	// Receiving message
	if ((recv(sockfd, buff, MSG_LEN, 0) <= 0)) {
		*num_clients=*num_clients-1;
		printf("Client disconnected. Total clients: %d\n", *num_clients);
		close(sockfd);
		fds[i].fd=0;
		return(-1);
		perror("recv");
		return(-1);
	}

	printf("Received: %s", buff);

	//If message = "/quit"
	if (strcmp(buff, "/quit\n") == 0) {
		printf("Client requested to close the connection. Closing...\n");
		
		// Sending closing message (ECHO)
		if (send(sockfd, buff, strlen(buff), 0) <= 0) {
			perror("send");
			return(-1);
		}

		printf("Closing message sent!\n");
		
		*num_clients=*num_clients-1;
		printf("Client disconnected. Total clients: %d\n", *num_clients);
		close(sockfd);
		fds[i].fd=0;
        fds[i].events=0;
        fds[i].revents=0;
		return(0);
	}

	// Sending message (ECHO)
	if (send(sockfd, buff, strlen(buff), 0) <= 0) {
		perror("send");
		return(-1);
	}

	printf("Message sent!\n");
	return(1);
		
}


int handle_bind(char* server_port) {
    // Déclaration des variables
    struct addrinfo hints, *result, *rp;
    int sfd;  // Descripteur de fichier du socket

    // Initialisation de la structure hints pour obtenir des informations sur l'adresse
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;   // IPv4 ou IPv6
    hints.ai_socktype = SOCK_STREAM;  // Socket de type flux (TCP)
    hints.ai_flags = AI_PASSIVE;  // Utilisation pour un serveur

    // Obtenir des informations sur l'adresse à partir du port et des indications fournies
    if (getaddrinfo(NULL, server_port, &hints, &result) != 0) {
        perror("getaddrinfo()");  // En cas d'erreur lors de l'obtention des informations sur l'adresse
        exit(EXIT_FAILURE);
    }

    // Parcourir la liste des résultats possibles
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        // Créer un socket en utilisant les informations fournies
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        
        if (sfd == -1) {
            continue;  // Si la création du socket échoue, essayer avec les informations suivantes
        }
        
        // Liaison du socket à l'adresse et au port spécifiés
        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;  // Si la liaison est réussie, sortir de la boucle
        }
        
        // En cas d'échec de la liaison, fermer le socket
        close(sfd);
    }

    // Si rp est NULL, cela signifie que la liaison a échoué pour toutes les informations
    if (rp == NULL) {
        fprintf(stderr, "Could not bind\n");  // Affichage d'un message d'erreur
        exit(EXIT_FAILURE);  // Sortie du programme avec un code d'erreur
    }

    // Libération de la mémoire allouée pour les informations sur l'adresse
    freeaddrinfo(result);

    // Renvoie le descripteur de fichier du socket lié avec succès
    return sfd;
}




int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_port = argv[1];

    int sfd = handle_bind(server_port);

    if ((listen(sfd, SOMAXCONN)) != 0) {
        perror("listen()\n");
        exit(EXIT_FAILURE);
    }

	//initialisation du nombre de client
	int num_clients=0;

    // Initialisation du tableau de structures pollfd
    int max_conn = 256;
    struct pollfd fds[max_conn];
    memset(fds, 0, max_conn * sizeof(struct pollfd));

    // Insertion de listen_fd dans le tableau
    fds[0].fd = sfd;
    fds[0].events = POLLIN;

	struct Client * chaine_cli = NULL;

    while (1) {
        // Appel à poll pour attendre de nouveaux événements
        int active_fds = poll(fds, max_conn, -1);
        if (active_fds<0){
			perror("poll");
            exit(EXIT_FAILURE);
		}

        for (int i = 0; i < max_conn; i++) {
            // S'il y a de l'activité sur fds, accepter une nouvelle connexion
            if (fds[i].fd == sfd && (fds[i].revents & POLLIN) == POLLIN) {
                
                // Accepter la nouvelle connexion et ajouter le nouveau descripteur de fichier dans le tableau fds
                struct sockaddr_in client_addr;
                socklen_t len = sizeof(client_addr);
                int new_fd = accept(sfd, (struct sockaddr *)&client_addr, &len);
                if (new_fd==-1){
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
				num_clients++;

				// Obtention de l'adresse IP du client
				char client_ip[INET_ADDRSTRLEN];
				if (inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN) == NULL) {
					perror("inet_ntop");
					exit(EXIT_FAILURE);
				}

				// Obtention du port du client
				int client_port = ntohs(client_addr.sin_port);

				//Création de la nouvelle structure client 
				struct Client *new_client=malloc(sizeof(struct Client));
				new_client->sockfd=new_fd;
				new_client->addr=client_addr.sin_addr;
				new_client->port=client_addr.sin_port;
                new_client->next=NULL;

                //Cas premier client
				if (chaine_cli==NULL)
					chaine_cli=new_client;
                //Sinon on décale.
				else
					chaine_cli->next=new_client;
                    chaine_cli=new_client;
				


				printf("New connection from %s:%d (client n°%d) on socket %d\n", client_ip, client_port,num_clients,new_fd);

                for (int j = 0; j < max_conn; j++) {
                    if (fds[j].fd == 0) {
                        fds[j].fd = new_fd;
                        fds[j].events = POLLIN;
						fds[j].revents = 0;
						break;
                    }
                }
				fds[i].revents = 0;
            }

            // S'il y a de l'activité sur un socket autre que listen_fd, lire les données
            if (fds[i].fd != sfd && (fds[i].revents & POLLIN) == POLLIN) {
                printf("Activity on socket %d\n", fds[i].fd);
                int ret=echo_server(&num_clients,fds,i)  ;
                if (ret==-1){
                    perror("echo_server");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    close(sfd);
    return EXIT_SUCCESS;
}
