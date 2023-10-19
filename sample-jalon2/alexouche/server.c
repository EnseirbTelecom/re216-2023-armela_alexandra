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
#include <time.h>

#include "common.h"
#include "msg_struct.h"

#define MAX_CONN 128

//Gère la déconnexion avec suppression de la liste chainée!
//chaine client head voir initialisation

struct Client {
    int sockfd;
	char* nickname;
    struct in_addr addr;
	unsigned short int port;
	time_t connectionTime;
    struct Client * next;
};

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
	// Sending message 
	if (send(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
		perror("send");
		return 0;
	}
	printf("Message sent!\n");
	return 1;
}

int is_connected_client(struct Client * chaine_cli_head, char* client){
	struct Client * current=chaine_cli_head;
    while (current!=NULL) {
        if (strcmp(current->nickname, client) == 0) {
            return 1; 
        }
		current=current->next;
    }
    return 0; 
}


int nickname_new(int sockfd,char nick_sender[NICK_LEN],char infos[INFOS_LEN],struct Client * chaine_cli_head ){
	//Verification of unassigned nickname
	if (is_connected_client(chaine_cli_head, infos)==1){
		char buff[MSG_LEN]="Nickname already attribuate";
		return send_struct(sockfd,"\0",NICKNAME_NEW,nick_sender,buff);
	}

	//Update client connected list
	struct Client * current=chaine_cli_head;

    while (current!=NULL) {
        if (current->sockfd==sockfd) {
			current->nickname=infos;
            printf("Username has been updated.\n"); 
        }
		current=current->next;
    }

	//Reply to the client
	//First connexion 
	if (strcmp(nick_sender, "\0") == 0){
		char buff[MSG_LEN]="Welcome on the chat ";
		strcat(buff, infos); 
		return send_struct(sockfd,"\0",NICKNAME_NEW,infos,buff);
	}

	//Update username
	char buff[MSG_LEN]="Your username has been updated.";
	strcat(buff, infos); 
	return send_struct(sockfd,"\0",NICKNAME_NEW,infos,buff);
}

int nickname_list(int sockfd,struct Client * chaine_cli_head ){
	struct Client * current=chaine_cli_head;

	//List of clients who are online
	char buff[MSG_LEN];
    while (current!=NULL) {
		if (strcmp(current->nickname,"\0")) //à verifier cette condition
			strcat(buff,current->nickname); 
		current=current->next;
    }

	return send_struct(sockfd,"\0",NICKNAME_LIST,"\0",buff);
}

int nickname_infos(int sockfd,char infos[INFOS_LEN],struct Client * chaine_cli_head){

	//Client connectivity check
	if (is_connected_client(chaine_cli_head, infos)==1){
		char buff[MSG_LEN]="User : ";
		strcat(buff,infos); 
		strcat(buff,"does not exist");
		return send_struct(sockfd,"\0",NICKNAME_INFOS,"\0",buff);
	}

	//Find information about the client
	struct Client * current=chaine_cli_head;
    while (current!=NULL) {
		if (current->sockfd==sockfd) 
			break;
		current=current->next;
    }

	char buff[MSG_LEN];
	//Format the time_t into a character string.
    strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", localtime(&current->connectionTime));

	//Format the struct in_addr into a character string.
	char addrStr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &((current->addr).s_addr), addrStr, INET_ADDRSTRLEN);

	//Format the unsigned short int into a character string.
	char portStr[6]; 
	sprintf(portStr, "%u", current->port);

	strcat(buff,addrStr);
	strcat(buff,portStr); 

	return send_struct(sockfd,"\0",NICKNAME_INFOS,infos,buff);
}

int broadcast_send(int sockfd,char nick_sender[NICK_LEN],enum msg_type type,char infos[INFOS_LEN],char buff[MSG_LEN],struct Client * chaine_cli_head){
	struct Client * current=chaine_cli_head;

	//Send to all connected clients except the sender.
	while (current!=NULL) {
		if (current->sockfd!=sockfd){
			if (send_struct(current->sockfd,nick_sender,type,infos,buff)==0)
				return 0;
		}
		current=current->next;
    }
	return 1;
}

int unicast_send(int sockfd,char nick_sender[NICK_LEN],enum msg_type type,char infos[INFOS_LEN],char buff[MSG_LEN],struct Client * chaine_cli_head){
	struct Client * current=chaine_cli_head;

	//Send to all connected clients except the sender.
	while (current!=NULL) {
		if (strcmp(current->nickname, infos) == 0)
			return send_struct(current->sockfd,nick_sender,type,infos,buff) ;
		current=current->next;
    }
	return 0;
}

int echo_send(int sockfd,int * num_clients,int i,struct pollfd * fds,enum msg_type type,char infos[INFOS_LEN],char buff[MSG_LEN],struct Client * chaine_cli_head){

	//If message = "/quit"
	if (strcmp(buff, "/quit\n") == 0) {
		printf("Client requested to close the connection. Closing...\n");
		
		// Sending closing message 
		if (send_struct(sockfd,"\0",type,infos,buff)==0)
			return 0;

		printf("Closing message sent!\n");
		
		
		*num_clients=*num_clients-1;
		printf("Client disconnected. Total clients: %d\n", *num_clients);

		

		struct Client * current=chaine_cli_head;
		struct Client * previous=chaine_cli_head;


		//Delete the client 
		while (current!=NULL) {
			if (current->sockfd==sockfd){
				if (current->sockfd==previous->sockfd){
					chaine_cli_head=current->next;
					break;
				}
				previous->next=current->next;
				break;
			}
			current=current->next;
    	}

		//Close the socket
		close(sockfd);
		fds[i].fd=0;
        fds[i].events=0;
        fds[i].revents=0;

		return 1;
	}

	return send_struct(sockfd,"\0",type,infos,buff);
}


int echo_server(int* num_clients,struct pollfd * fds,int i,struct Client * chaine_cli_head) {
	int sockfd=fds[i].fd;
	
	struct message msgstruct;
	char buff[MSG_LEN];

	// Cleaning memory
	memset(&msgstruct, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN);

	// Receiving structure
	if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
		perror("recv");
		return 0;
	}

	// Receiving message
	if (recv(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
		perror("recv");
		return 0;
	}

	printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
	

	//NICKNAME_NEW
	if (msgstruct.type == NICKNAME_NEW)
		return nickname_new(sockfd,msgstruct.nick_sender,msgstruct.infos,chaine_cli_head);
	
	//The client has no nickname yet
	if (strcmp(msgstruct.nick_sender, "\0") == 0){
		char buff[MSG_LEN]="please login with /nick <your pseudo>";
		return send_struct(sockfd,"\0",NICKNAME_NEW,"/0",buff);
	}

	//NICKNAME_LIST
	if (msgstruct.type == NICKNAME_LIST)
		return nickname_list(sockfd,chaine_cli_head);
	
	//NICKNAME_INFOS
	if (msgstruct.type == NICKNAME_INFOS)
		return nickname_infos(sockfd,msgstruct.infos,chaine_cli_head);
	
	//BROADCAST_SEND
	if (msgstruct.type == BROADCAST_SEND)
		return broadcast_send(sockfd,msgstruct.nick_sender,msgstruct.type,msgstruct.infos,buff,chaine_cli_head);
	
	//UNICAST_SEND
	if (msgstruct.type == UNICAST_SEND)
		return unicast_send(sockfd,msgstruct.nick_sender,msgstruct.type,msgstruct.infos,buff,chaine_cli_head);

	//ECHO_SEND
	if (msgstruct.type == ECHO_SEND)
		return echo_send(sockfd,num_clients,i,fds,msgstruct.type,msgstruct.infos,buff,chaine_cli_head);

	return 0;
}
	


int handle_bind(char* server_port)  {
	struct addrinfo hints, *result, *rp;
	int sfd;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(NULL, SERV_PORT, &hints, &result) != 0) {
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
    struct pollfd fds[MAX_CONN];
    memset(fds, 0, MAX_CONN * sizeof(struct pollfd));

    // Insertion de listen_fd dans le tableau
    fds[0].fd = sfd;
    fds[0].events = POLLIN;

	
	struct Client * chaine_cli = NULL;
	struct Client * chaine_cli_head = NULL;
	
	while (1) {
        // Appel à poll pour attendre de nouveaux événements
        int active_fds = poll(fds, MAX_CONN, -1);
        if (active_fds<0){
			perror("poll");
            exit(EXIT_FAILURE);
		}

        for (int i = 0; i < MAX_CONN; i++) {
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
				new_client->connectionTime = time(NULL);
				new_client->nickname="\0";
                new_client->next=NULL;

				char buff[MSG_LEN]="please login with /nick <your pseudo>";
				if(send_struct(new_fd,"\0",NICKNAME_NEW,"\0",buff)==0)
					exit(EXIT_FAILURE);

                //Cas premier client
				if (chaine_cli==NULL){
					chaine_cli=new_client;
					chaine_cli_head=new_client;
				}
                //Sinon on décale.
				else{
					chaine_cli->next=new_client;
                    chaine_cli=new_client;
				}

				printf("New connection from %s:%d (client n°%d) on socket %d\n", client_ip, client_port,num_clients,new_fd);

                for (int j = 0; j < MAX_CONN; j++) {
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
				int ret=echo_server(&num_clients,fds,i,chaine_cli_head)  ;
				if (ret==0){
                    perror("echo_server");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    close(sfd);
    return EXIT_SUCCESS;
}












/*

int ask_nickname(int sockfd){
	struct message msgstruct;
	char buff[MSG_LEN]="please login with /nick <your pseudo>";
	// Filling structure
	msgstruct.pld_len = strlen(buff);
	strncpy(msgstruct.nick_sender, my_nickname, strlen(my_nickname));
	msgstruct.type = NICKNAME_NEW;
	strncpy(msgstruct.infos, "\0", 1);

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
	printf("Ask nickname send\n");
	return 1;
}

int nickname_already_attribuate(char * my_nickname,int sockfd){
	struct message msgstruct;
	char buff[MSG_LEN]="Nickname already attribuate";
	// Filling structure
	msgstruct.pld_len = strlen(buff);
	strncpy(msgstruct.nick_sender, my_nickname, strlen(my_nickname));
	msgstruct.type = NICKNAME_NEW;
	strncpy(msgstruct.infos, "\0", 1);

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
	printf("send\n");
	return 1;
}
*/


/*
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
*/



