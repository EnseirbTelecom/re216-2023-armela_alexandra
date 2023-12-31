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
#include <ctype.h>

#include "common.h"
#include "msg_struct.h"

//CONNEXION 
int handle_bind(char* server_port) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, server_port, &hints, &result) != 0) {
		perror("getaddrinfo()");
		exit(EXIT_FAILURE);
	}
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
		rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
			break;
		}
		close(sfd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not bind\n");
		exit(EXIT_FAILURE);
	}
	freeaddrinfo(result);
	return sfd;
}

int received_file(int sockfd){
	struct message msgstruct;
	char buff[MSG_LEN];

	// Cleaning memory
	memset(&msgstruct, 0, sizeof(struct message));

	// Receiving structure
	size_t totalStructBytesReceived = 0;
	while (totalStructBytesReceived < sizeof(struct message)) {
		ssize_t structBytesReceived = recv(sockfd, ((char*)&msgstruct) + totalStructBytesReceived, sizeof(struct message) - totalStructBytesReceived, 0);
		if (structBytesReceived <= 0) {
			perror("Erreur lors de la réception de la structure");
			return 0;  
		}
		totalStructBytesReceived += structBytesReceived;
	}

	//Show the struct
	//printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
	
	char repo[INFOS_LEN]="inbox/";
	strcat(repo, msgstruct.infos);
	printf("repo :%s\n",repo);
    int fd=open(repo, O_RDWR|O_CREAT|O_TRUNC,S_IRWXU);
    if (fd == -1) {
        perror("Erreur lors de l'ouverture du fichier");
        return 0;
    }

	int nb_buff = msgstruct.pld_len / MSG_LEN +1;

	for (int i = 0; i < nb_buff; i++){
		memset(buff, 0, MSG_LEN);
        int totalMsgBytesReceived=0;
        int bytesToRecv;
        if (i==nb_buff-1)
            bytesToRecv=msgstruct.pld_len % MSG_LEN;
        else
            bytesToRecv=MSG_LEN;

		while (totalMsgBytesReceived < bytesToRecv) {
			ssize_t msgBytesReceived = recv(sockfd, buff + totalMsgBytesReceived, bytesToRecv - totalMsgBytesReceived, 0);
			if (msgBytesReceived <= 0) {
				perror("Erreur lors de la réception du champ buff");
				return 0;  
			}
			totalMsgBytesReceived += msgBytesReceived;
		}

		int bytesWrite=0;
		while (bytesWrite <bytesToRecv ) {
			int offset=write(fd, buff+bytesWrite, bytesToRecv-bytesWrite);
			if (offset<=0){
				perror("Erreur lors de l'écriture buff");
				return 0;  
			}
			bytesWrite+=offset;
		}
        
	}
    close(fd);
	return 1;

}

int main(int argc, char *argv[]){
    
	if (argc != 2) {
        fprintf(stderr, "Usage: %s <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

	char *server_port = argv[1];


    struct sockaddr cli;
	int sfd, connfd;
	socklen_t len;

    sfd = handle_bind(server_port);


    if ((listen(sfd, SOMAXCONN)) != 0) {
		perror("listen()\n");
		return 0 ;
	}

	len = sizeof(cli);
	if ((connfd = accept(sfd, (struct sockaddr*) &cli, &len)) < 0) {
		perror("accept()\n");
		return 0 ;
	}

	printf("... Connecting to client ... done!\n");

	received_file(connfd);
	close(sfd);
	exit(EXIT_SUCCESS);

}

