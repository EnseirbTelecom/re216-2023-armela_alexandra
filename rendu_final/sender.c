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

// AUXILIARY FUNCTIONS
long long get_file_size(const char *filename) {
    FILE *file = fopen(filename, "rb"); // Ouvrir le fichier en mode lecture binaire
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier");
        return -1; // Ou toute autre valeur indiquant une erreur
    }

    fseek(file, 0, SEEK_END); // Se déplacer à la fin du fichier
    long long size = ftell(file); // Récupérer la position courante, qui est la taille du fichier
    fclose(file); // Fermer le fichier

    return size;
}

// CONNEXION
int handle_connect(char* client_port,char* client_addr) {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(client_addr, client_port, &hints, &result) != 0) {
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

int send_file(int sockfd, const char *nick_sender, char *infos) {
    struct message msgstruct;

    long long file_size = get_file_size(infos);
    
    char *lastSlash = strrchr(infos, '/');
    // If the '/' character is found, advance one position to obtain the file name
    if (lastSlash != NULL) {
        lastSlash++; 
    } else {
        lastSlash = infos;
    }

    // Filling structure
    msgstruct.pld_len = file_size;
    memset(msgstruct.nick_sender, 0, NICK_LEN);
    strncpy(msgstruct.nick_sender, nick_sender, strlen(nick_sender));
    msgstruct.type = FILE_SEND;
    memset(msgstruct.infos, 0, INFOS_LEN);
    strncpy(msgstruct.infos, lastSlash, strlen(lastSlash));

    // Set a socket timeout
    struct timeval timeout;
    timeout.tv_sec = 10;  
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Erreur lors de la configuration du délai d'attente de la socket");
        return 1;
    }
    
    // Sending structure
    size_t totalStructBytesSent = 0;
    while (totalStructBytesSent < sizeof(msgstruct)) {
        ssize_t structBytesSent = send(sockfd, ((char *)&msgstruct) + totalStructBytesSent, sizeof(msgstruct) - totalStructBytesSent, 0);
        if (structBytesSent <= 0) {
            perror("Erreur lors de l'envoi de la structure");
            return 1; 
        }
        totalStructBytesSent += structBytesSent;
    }

    // Sending file content
    int fd=open(infos, O_RDWR);
    if (fd == -1) {
        perror("Erreur lors de l'ouverture du fichier");
        return 1;
    }

    char buffer[MSG_LEN];
	int nb_buff = file_size / MSG_LEN +1;
    
	for (int i = 0; i < nb_buff; i++){
        memset(buffer, 0, MSG_LEN);
		int bytesRead=0;
        int bytesToRead;
        if (i==nb_buff-1)
            bytesToRead=file_size % MSG_LEN;
        else
            bytesToRead=MSG_LEN;

		while (bytesRead < bytesToRead ) {
			int offset=read(fd, buffer+bytesRead, bytesToRead- bytesRead);
			if (offset<=0){
				perror("Erreur lors de la lecture buff");
				return 1;  
			}
			bytesRead+=offset;
		}

		size_t totalBytesSent = 0;
		while (totalBytesSent < bytesToRead) {
			ssize_t bytesSent = send(sockfd, buffer + totalBytesSent, bytesRead - totalBytesSent, 0);
			if (bytesSent <= 0) {
				perror("Erreur lors de l'envoi du champ buff");
				return 1;  
			}
			totalBytesSent += bytesSent;
		}
	}
    close(fd);
    return 1; // Succes
}

int main(int argc, char *argv[]){
    sleep(2);
	if (argc != 5) {
        fprintf(stderr, "Usage: %s <server_name> <server_port> <nickname> <filepath>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

	char * client_name=argv[1];
	char * client_port=argv[2];
    char * nickname=argv[3];
    char * filepath=argv[4];

    int sfd;
	sfd = handle_connect(client_port,client_name);
	
    printf("... Connecting to client ... done!\n");
    
    send_file(sfd,nickname,filepath);
	
    close(sfd);

	exit(EXIT_SUCCESS);
}
