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

int echo_client(int sockfd_active,int sockfd_server, int sockfd_entree) {

	char buff[MSG_LEN];
	int n;

	if (sockfd_active==sockfd_entree)
	{
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		// Getting message from client
		n = 0;
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent
		// Sending message (ECHO)
		if (send(sockfd_server, buff, strlen(buff), 0) <= 0) {
			perror("send");
			return -1;
		}
		printf("Message sent!\n");
	}

	if (sockfd_active==sockfd_server)
	{
		// Cleaning memory
		memset(buff, 0, MSG_LEN);
		// Receiving message
		if (recv(sockfd_server, buff, MSG_LEN, 0) <= 0) {
			perror("recv");
			return -1;
		}
		if (strcmp(buff, "/quit\n") == 0) {
            printf("Server accepted the request to close the connection. Closing...\n");
            close(sockfd_server);  // Ferme la connexion du client.
            exit(EXIT_SUCCESS);  // Arrête le programme client.
        }

		printf("Received: %s", buff);

	}
	return 0;
}

int handle_connect(char * server_name,char * server_port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(server_name, server_port, &hints, &result) != 0) {
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

int main(int argc, char *argv[]) {
	if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_name> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

	char * server_name=argv[1];
	char * server_port=argv[2];

	int sfd;
	sfd = handle_connect(server_name,server_port);
	

    // Initialisation du tableau de structures pollfd
    int max_conn = 2;
    struct pollfd fds[max_conn];
    memset(fds, 0, max_conn * sizeof(struct pollfd));

    // Insertion de sfd dans le tableau
    fds[0].fd = sfd;
    fds[0].events = POLLIN;

	// Insertion de l'entrée standars dans le tableau
    fds[1].fd = fileno(stdin);
    fds[1].events = POLLIN;

	int ret;

	 while (1) {
        // Appel à poll pour attendre de nouveaux événements
        int active_fds = poll(fds, max_conn, -1);
		if (active_fds<0){
			perror("poll");
            exit(EXIT_FAILURE);
		}
		

		//si il y a de l'activité sur la socket liée au serveur
		if ((fds[0].revents & POLLIN) == POLLIN) {
			ret=echo_client(fds[0].fd,fds[0].fd,fds[1].fd);
			if (ret<-1){
				perror("echo_client");
				exit(EXIT_FAILURE);
			}
			
		}

		//si il y a de l'activité sur la socket de l'entrée standard
		if ((fds[1].revents & POLLIN) == POLLIN) {
			ret=echo_client(fds[1].fd,fds[0].fd,fds[1].fd);
			if (ret==-1){
				perror("echo_client");
				exit(EXIT_FAILURE);
			}
		}
	 }
	close(sfd);
	return EXIT_SUCCESS;
}

