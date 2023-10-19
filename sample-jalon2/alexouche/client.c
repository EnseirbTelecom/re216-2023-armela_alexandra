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

int containsOnlyAlphanumeric(char str[]) {
	printf("%lu\n",strlen(str));
    for (int i = 0; i < strlen(str)-1; i++)
	{
		if(!isalpha(str[i])&&!isdigit(str[i]))
			return 0;
	}
	
    return 1; // Tous les caractères sont des chiffres ou des lettres
}

int send_message(int sockfd,char nick_sender[NICK_LEN],enum msg_type type,char infos[INFOS_LEN],char buff[MSG_LEN]){
	struct message msgstruct;
	// Filling structure
	msgstruct.pld_len = strlen(buff);
	strncpy(msgstruct.nick_sender, nick_sender, strlen(nick_sender));
	msgstruct.type = type;
	strncpy(msgstruct.infos, infos, strlen(infos));

	// Sending structure
	if (send(sockfd, &msgstruct, sizeof(msgstruct), 0) <= 0) {
		perror("send");
		return 0;
	}
	// Sending message (ECHO)
	if (send(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
		perror("send");
		return 0;
	}
	printf("Message sent!\n");
	return 1;
}


int nick(char buff[MSG_LEN],int len_order,int n,int sockfd_server,char** my_nickname){
	//Récupère le nouveau nickname
    char new_nickname[256];
    strncpy(new_nickname, buff+len_order, n-len_order);

    // Terminer le nouveau nickname avec un caractère nul
    new_nickname[n-len_order] = '\0';

	if ((n-len_order<3)||(n-len_order>127)){
		printf("The nickname size is incorrect; it should be between 3 and 127 characters.");
		return 0;
	}

	if (containsOnlyAlphanumeric(new_nickname)==0){
		printf("The nickname should only contain letters of the alphabet or numbers.");
		return 0;
	}

	if(send_message(sockfd_server,*my_nickname,NICKNAME_NEW,new_nickname,"\0")==0)
		return 0;
			
	return 1;
}


int who(char buff[MSG_LEN],int len_order,int n,int sockfd_server,char** my_nickname){

	if(send_message(sockfd_server,*my_nickname,NICKNAME_LIST,"\0","\0")==0)
		return 0;
			
	return 1;
}

int whois(char buff[MSG_LEN],int len_order,int n,int sockfd_server,char** my_nickname){
	//Récupère le nouveau nickname
    char nickname[256];
    strncpy(nickname, buff+len_order, n-len_order);

    // Terminer le nouveau nickname avec un caractère nul
    nickname[n-len_order] = '\0';

	if ((n-len_order<3)||(n-len_order>127)){
		printf("The nickname size is incorrect; it should be between 3 and 127 characters.");
		return 0;
	}

	if (containsOnlyAlphanumeric(nickname)==0){
		printf("The nickname should only contain letters of the alphabet or numbers.");
		return 0;
	}

	if(send_message(sockfd_server,*my_nickname,NICKNAME_NEW,nickname,"\0")==0)
		return 0;
			
	return 1;
}

int msgall(char buff[MSG_LEN],int len_order,int n,int sockfd_server,char** my_nickname){
	//Récupère le message
    char msg[256];
    strncpy(msg, buff+len_order, n-len_order);

    // Terminer le message
    msg[n-len_order] = '\0';

	if(send_message(sockfd_server,*my_nickname,NICKNAME_NEW,"\0",msg)==0)
		return 0;
			
	return 1;
}

int msg(char buff[MSG_LEN],int len_order,int n,int sockfd_server,char** my_nickname){
	//Récupère le nouveau nickname
    char nickname[256];
    strncpy(nickname, buff+len_order, n-len_order);

    // Terminer le nouveau nickname avec un caractère nul
    nickname[n-len_order] = '\0';

	if ((n-len_order<3)||(n-len_order>127)){
		printf("The nickname size is incorrect; it should be between 3 and 127 characters.");
		return 0;
	}

	if (containsOnlyAlphanumeric(nickname)==0){
		printf("The nickname should only contain letters of the alphabet or numbers.");
		return 0;
	}

	if(send_message(sockfd_server,*my_nickname,NICKNAME_NEW,nickname,"\0")==0)
		return 0;
			
	return 1;
}



int echo_client(int sockfd_active,int sockfd_server, int sockfd_entree,char** my_nickname) {
	struct message msgstruct;
	char buff[MSG_LEN];
	int n;

	if (sockfd_active==sockfd_entree)
	{
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
		// Getting message from client
		n = 0;
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent

		//NICKNAME_NEW
		char* order= "/nick ";
		int len_order=strlen(order);

		if (strncmp(buff, order, len_order))
			return nick(buff,len_order,n,sockfd_server,my_nickname);
					
			
		//NICKNAME_LIST
		char* order= "/who ";
		int len_order=strlen(order);

		if (strncmp(buff, order, len_order))
			return who(buff,len_order,n,sockfd_server,my_nickname);
		
		//NICKNAME_IN
		char* order= "/whois ";
		int len_order=strlen(order);

		if (strncmp(buff, order, len_order))
			return whois(buff,len_order,n,sockfd_server,my_nickname);
		
		//UNICAST_SEND
		char* order= "/msgall ";
		int len_order=strlen(order);

		if (strncmp(buff, order, len_order))
			return msgall(buff,len_order,n,sockfd_server,my_nickname);
		
		//UNICAST_SEND
		char* order= "/msg ";
		int len_order=strlen(order);

		if (strncmp(buff, order, len_order))
			return msg(buff,len_order,n,sockfd_server,my_nickname);
		

		

		/*
		// Filling structure
		msgstruct.pld_len = strlen(buff);
		strncpy(msgstruct.nick_sender, my_nickname, strlen(my_nickname));
		msgstruct.type = ECHO_SEND;
		strncpy(msgstruct.infos, "\0", 1);
		// Sending structure
		if (send(sockfd_server, &msgstruct, sizeof(msgstruct), 0) <= 0) {
			perror("send");
			return 0;
		}
		// Sending message (ECHO)
		if (send(sockfd_server, buff, msgstruct.pld_len, 0) <= 0) {
			perror("send");
			return 0;
		}
		printf("Message sent!\n");
		*/
	}
	if (sockfd_active==sockfd_server)
	{
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);

		// Receiving structure
		if (recv(sockfd_server, &msgstruct, sizeof(struct message), 0) <= 0) {
			perror("recv");
			return 0;
		}
		// Receiving message
		if (recv(sockfd_server, buff, msgstruct.pld_len, 0) <= 0) {
			perror("recv");
			return 0;
		}

		printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
		printf("[Server]: %s \n", buff);

		//Received ECHO_SEND
		if (msgstruct.type == ECHO_SEND){
			printf("Server received the message\n");
			return 1;
		}
		
		//Send ECHO_SEND
		if(send_message(sockfd_server,*my_nickname,ECHO_SEND,"/0","/0")==0)
			return 0;


		/* Demande fermeture de connexion
		if (strcmp(buff, "/quit\n") == 0) {
            printf("Server accepted the request to close the connection. Closing...\n");
            close(sockfd_server);  // Ferme la connexion du client.
            exit(EXIT_SUCCESS);  // Arrête le programme client.
        }
		*/
	}	
	return 1;
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
	printf("Connecting to server ... done!\n");
	

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
	char *my_nickname=malloc(NICK_LEN*sizeof(char));

	while (1) {
        // Appel à poll pour attendre de nouveaux événements
        int active_fds = poll(fds, max_conn, -1);
		if (active_fds<0){
			perror("poll");
            exit(EXIT_FAILURE);
		}
		

		//si il y a de l'activité sur la socket liée au serveur
		if ((fds[0].revents & POLLIN) == POLLIN) {
			ret=echo_client(fds[0].fd,fds[0].fd,fds[1].fd,&my_nickname);
			if (ret==0){
				perror("echo_client");
				exit(EXIT_FAILURE);
			}
			
		}

		//si il y a de l'activité sur la socket de l'entrée standard
		if ((fds[1].revents & POLLIN) == POLLIN) {
			ret=echo_client(fds[1].fd,fds[0].fd,fds[1].fd,&my_nickname);
			if (ret==0){
				perror("echo_client");
				exit(EXIT_FAILURE);
			}
		}
	 }
	close(sfd);
	return EXIT_SUCCESS;
}

