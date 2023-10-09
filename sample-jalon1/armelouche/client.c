#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"

//envoyer des messages au serveur et recevoir des réponses du serveur en bpucle continue 

//sockfd : descripyeur de socket vers le serveur auquel le client est connecté 
void echo_client(int sockfd) {
	
	char buff[MSG_LEN];
	int n;
	while (1) {
		
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		
		// lire le message de l'utilisateur depuis la console
		printf("Message: ");
		n = 0;
		
		// lire les carcatères saisis par l'utilisateur: les caractères sont sockés dans buff
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
		
		// Sending message (ECHO) : envoi le contenu du buff ua serveur
		if (send(sockfd, buff, strlen(buff), 0) <= 0) {
			break;
		}
		printf("Message sent!\n");
		
		// Cleaning memory: pour le préparer à la reception d'une réponse
		memset(buff, 0, MSG_LEN);
		
		// Receiving message : attend la réponse où que la connexion avec les erveur soit fait
		if (recv(sockfd, buff, MSG_LEN, 0) <= 0) {
			break;
		}
		printf("Received: %s", buff);
	}
}


// établir une connexion réseau à un serveur distant en utilisant les sockets 

int handle_connect(const char *server_addr, const char *server_port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(server_addr, server_port, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
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

int main(int argc, char*argv[]) {
	
	// verifier les arguments passés en paramètre 
	if (argc !=3){
		printf("Arguments invalides !\n");
	}
	
	const char *server_name = argv[1];
	const char *server_port = argv[2];

	int sfd;
	sfd = handle_connect(server_name, server_port);
	echo_client(sfd);
	close(sfd);
	return EXIT_SUCCESS;
}

