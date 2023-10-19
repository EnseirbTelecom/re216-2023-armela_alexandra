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
    for (int i = 0; i < strlen(str)-1; i++)
	{
		//Un caractère n'est ni un chiffre ni une lettre
		if(!isalpha(str[i])&&!isdigit(str[i]))
			return 0;
	}
	
    return 1; 
}


//Envoyer une structure
int send_struct(int sockfd,char nick_sender[NICK_LEN],enum msg_type type,char infos[INFOS_LEN],char buff[MSG_LEN]){
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


// FONCTIONS POUR LE TRAITEMENT D'INFORMATIONS AU CLAVIER
int nick(char buff[MSG_LEN],int sockfd_server,char** my_nickname){
	char *infos = strtok(buff, " ");

	if (infos== NULL){
		printf("No space between /nick and <your pseudo> \n");
		return 0;
	}

	//Récupère le nouveau nickname
	infos = strtok(NULL, "");

	if (infos== NULL){
		printf("No nickname\n");
		return 0;
	}

	char* new_nickname=infos;
	int len_nickname=strlen(new_nickname);

	
	if ((len_nickname<3)||(len_nickname>127)){
		printf("The new nickname size is incorrect; it should be between 3 and 127 characters.");
		return 0;
	}

	if (containsOnlyAlphanumeric(new_nickname)==0){
		printf("The new nickname should only contain letters of the alphabet or numbers.");
		return 0;
	}

	return send_struct(sockfd_server,*my_nickname,NICKNAME_NEW,new_nickname,"\0");
		
}


int who(int sockfd_server,char** my_nickname){

	if(send_struct(sockfd_server,*my_nickname,NICKNAME_LIST,"\0","\0")==0)
		return 0;
			
	return 1;
}

int whois(char buff[MSG_LEN],int sockfd_server,char** my_nickname){
	char *infos = strtok(buff, " ");

	if (infos== NULL){
		printf("No space between /whois and <pseudo> \n");
		return 0;
	}

	//Récupère le nickname
	infos = strtok(NULL, "");

	if (infos== NULL){
		printf("No nickname\n");
		return 0;
	}

	char* nickname=infos;
	int len_nickname=strlen(nickname);

	
	if ((len_nickname<3)||(len_nickname>127)){
		printf("The nickname size is incorrect; it should be between 3 and 127 characters.");
		return 0;
	}

	if (containsOnlyAlphanumeric(nickname)==0){
		printf("The nickname should only contain letters of the alphabet or numbers.");
		return 0;
	}
	return send_struct(sockfd_server,*my_nickname,NICKNAME_NEW,nickname,"\0");
		
}

int msgall(char buff[MSG_LEN],int sockfd_server,char** my_nickname){
	char *infos = strtok(buff, " ");

	if (infos== NULL){
		printf("No space between /msgall and <message> \n");
		return 0;
	}

	//Récupère le message
	infos = strtok(NULL, "");
	char* msg=infos;
	
	return send_struct(sockfd_server,*my_nickname,NICKNAME_NEW,"\0",msg);
		
}

int msg(char buff[MSG_LEN],int sockfd_server,char** my_nickname){
	char *infos = strtok(buff, " ");

	if (infos== NULL){
		printf("No space between <pseudo> and <message> \n");
		return 0;
	}

	//Récupère le nickname
	infos = strtok(NULL, " ");

	if (infos== NULL){
		printf("No nickname\n");
		return 0;
	}

	char* nickname=infos;
	int len_nickname=strlen(nickname);

	//Verification du nickname (taille/alphanumérique)
	if ((len_nickname<3)||(len_nickname>127)){
		printf("The nickname size is incorrect; it should be between 3 and 127 characters.");
		return 0;
	}

	if (containsOnlyAlphanumeric(nickname)==0){
		printf("The nickname should only contain letters of the alphabet or numbers.");
		return 0;
	}

	infos = strtok(NULL, "");

	if (infos== NULL){
		printf("No message\n");
		return 0;
	}

	char* msg=infos;

	return send_struct(sockfd_server,*my_nickname,UNICAST_SEND,nickname,msg);
}






int echo_client(int sockfd_active,int sockfd_server, int sockfd_entree,char** my_nickname) {
	struct message msgstruct;
	char buff[MSG_LEN];
	int n;

	//TRAITEMENT DES INFORMATIONS CLAVIER
	if (sockfd_active==sockfd_entree)
	{
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
		// Getting message from client
		n = 0;
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent

		//NICKNAME_NEW
		if (strncmp(buff, "/nick ", strlen("/nick ")))
			return nick(buff,sockfd_server,my_nickname);		
			
		//NICKNAME_LIST
		if (strncmp(buff, "/who ", strlen("/who ")))
			return who(sockfd_server,my_nickname);
		
		//NICKNAME_IN
		if (strncmp(buff, "/whois ", strlen("/whois ")))
			return whois(buff,sockfd_server,my_nickname);
		
		//UNICAST_SEND
		if (strncmp(buff, "/msgall ", strlen("/msgall ")))
			return msgall(buff,sockfd_server,my_nickname);
		
		//UNICAST_SEND
		if (strncmp(buff, "/msg ", strlen("/msg ")))
			return msg(buff,sockfd_server,my_nickname);
		
		//ECHO_SEND
		return send_struct(sockfd_server,*my_nickname,ECHO_SEND,"\0",buff);
		
	}


	//TRAITEMENT DES INFORMATIONS REÇUES DU SERVER
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

		printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n\n ", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
		

		//NICKNAME_NEW
		if (msgstruct.type == NICKNAME_NEW){
			printf("[Server]: %s \n", buff);
			*my_nickname=msgstruct.infos;
			return 1;
		}

		//NICKNAME_LIST
		if (msgstruct.type == NICKNAME_LIST){
			printf("[Server]: Online users are :\n");
			
    		char *connected_clients = strtok(buff, "");

   			while (connected_clients != NULL) {
        		printf("-%s\n", connected_clients);
        		connected_clients = strtok(NULL, "");
   			}
			return 1;
		}

		//NICKNAME_IN
		if (msgstruct.type ==NICKNAME_INFOS){
			printf("[Server]: %s connected ",msgstruct.infos);

    		char *infos_user = strtok(buff, "");
			//Date 
			printf("since %s ", infos_user);

			//IP address
        	infos_user = strtok(NULL, "");
			printf("with IP address %s ", infos_user);

			//Port number
			infos_user = strtok(NULL, "");
			printf("and port number %s \n", infos_user);
			return 1;
		}

		//UNICAST_SEND
		if (msgstruct.type ==UNICAST_SEND){
			printf("[%s]: %s \n", msgstruct.nick_sender,buff);
			return 1;
		}

		//BROADCAST_SEND
		if (msgstruct.type ==BROADCAST_SEND){
			printf("[%s]: %s \n", msgstruct.nick_sender,buff);
			return 1;
		}

		//ECHO_SEND
		if (msgstruct.type == ECHO_SEND){
			printf("[Server]: %s \n", buff);

			//Close connexion accepted
			if (strcmp(buff, "/quit\n") == 0) {
            	printf("Server accepted the request to close the connection. Closing...\n");
            	close(sockfd_server);  // Ferme la connexion du client.
            	exit(EXIT_SUCCESS);  // Arrête le programme client.
       		}

			return 1;
		}		
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
	char *my_nickname="\0";


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

