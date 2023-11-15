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


int send_struct(int sockfd,char nick_sender[NICK_LEN],enum msg_type type,char infos[INFOS_LEN],char buff[MSG_LEN]){
	struct message msgstruct;
	
	// Filling structure
	msgstruct.pld_len = strlen(buff);
	memset(msgstruct.nick_sender, 0, NICK_LEN);
	strncpy(msgstruct.nick_sender, nick_sender, strlen(nick_sender));
	msgstruct.type = type;
	memset(msgstruct.infos, 0, INFOS_LEN);
	strncpy(msgstruct.infos, infos, strlen(infos));

	// Sending structure
	size_t totalStructBytesSent = 0;
	while (totalStructBytesSent < sizeof(msgstruct)) {
		ssize_t structBytesSent = send(sockfd, ((char*)&msgstruct) + totalStructBytesSent, sizeof(msgstruct) - totalStructBytesSent, 0);
		if (structBytesSent <= 0) {
			perror("Erreur lors de l'envoi de la structure");
			return 0;  // Ou prendre d'autres mesures en cas d'échec d'envoi
		}
		totalStructBytesSent += structBytesSent;
	}

	// Sending message (ECHO)
	size_t totalBytesSent = 0;
	while (totalBytesSent < msgstruct.pld_len) {
		ssize_t bytesSent = send(sockfd, buff + totalBytesSent, msgstruct.pld_len - totalBytesSent, 0);
		if (bytesSent <= 0) {
			perror("Erreur lors de l'envoi du champ buff");
			return 0;  // Ou prendre d'autres mesures en cas d'échec d'envoi
		}
		totalBytesSent += bytesSent;
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

int is_existing_channel(struct Channel all_channel[MAX_CHANNEL], char* name_channel){
	for (int i = 0; i < MAX_CHANNEL; i++){
		if (strcmp(all_channel[i].name, name_channel) == 0) 
            return 1;
	}
    return 0;
}

void disp_chaine(struct Client * chaine_cli_head){
	printf("Affichage de la chaine : \n");
	struct Client * current=chaine_cli_head;
	while (current!=NULL) {
		printf("sock: %d\n",current->sockfd);
		sleep(2);
		current=current->next;
	}
}

void removeClient(struct Client** head, int sockfd) {
    struct Client* current = *head;
    struct Client* prev = NULL;


    // Traverse the linked list to find the client with the given 'sockfd'.
    while (current != NULL) {
        if (current->sockfd == sockfd) {
            if (prev == NULL) {
                if (current->next == NULL) {
                    // If the client to be removed is the only one in the list.
                    *head = NULL;
                } else {
                    // If the client to be removed is the head of the list.
                    // Update the head to point to the next client.
                    *head = current->next;
                }
            } else {
                // If the client to be removed is not the head.
                prev->next = current->next;
            }
            free(current->nickname); // Free the dynamically allocated nickname.
            free(current->salon);    // Free the dynamically allocated salon.
            free(current);           // Free the client struct itself.
            return;
        }
        prev = current;
        current = current->next;
    }
}

int nickname_new(int sockfd,char nick_sender[NICK_LEN],char infos[INFOS_LEN],struct Client * chaine_cli_head ){
	//Verification of unassigned nickname

	if (is_connected_client(chaine_cli_head, infos)==1){
		char buff[MSG_LEN]="Nickname already attribuate";
		return send_struct(sockfd,"",NICKNAME_NEW,nick_sender,buff);
	}

	//Update client connected list
	struct Client * current=chaine_cli_head;

    while (current!=NULL) {
        if (current->sockfd==sockfd) {
			current->nickname=strdup(infos);
            printf("Username has been updated : %s \n",current->nickname); 
			break;
        }
		current=current->next;
    }

	//Reply to the client
	//First connexion 
	if (nick_sender[0]=='\0'){
		char buff[MSG_LEN]="Welcome on the chat ";
		strcat(buff, infos); 
		return send_struct(sockfd,"\0",NICKNAME_NEW,infos,buff);
	}

	//Update username
	char buff[MSG_LEN]="Your username has been updated ";
	strcat(buff, infos); 
	return send_struct(sockfd,"\0",NICKNAME_NEW,infos,buff);
}

int nickname_list(int sockfd, struct Client *chaine_cli_head) {
    struct Client *current = chaine_cli_head;

    // Liste des clients en ligne
    char buff[MSG_LEN];
    memset(buff, 0, MSG_LEN);
    
    while (current != NULL) {
		if (current->nickname != NULL && current->nickname[0] != '\0') {
			strcat(buff, current->nickname);
			strcat(buff, "\n");
		}
		current = current->next;
	}

    return send_struct(sockfd, "", NICKNAME_LIST, "", buff);
}

int nickname_infos(int sockfd,char infos[INFOS_LEN],struct Client * chaine_cli_head){
	char buff[MSG_LEN];
	memset(buff,0,MSG_LEN);
	//Client connectivity check
	if (is_connected_client(chaine_cli_head, infos)==0){
		strcpy(buff,"not connected");
		return send_struct(sockfd,"\0",NICKNAME_INFOS,infos,buff);
	}

	//Find information about the client
	struct Client * current=chaine_cli_head;
    while (current!=NULL) {
		if (current->sockfd==sockfd) 
			break;
		current=current->next;
    }

	// Convertir l'adresse IP en une chaîne de caractères lisible
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(current->addr), ip_str, INET_ADDRSTRLEN);

    // Convertir le temps de connexion en une chaîne de caractères
    char time_str[20]; // Assez grand pour contenir une date au format "YYYY/MM/DD@HH:MM"
    strftime(time_str, sizeof(time_str), "%Y/%m/%d@%H:%M", localtime(&(current->connectionTime)));

    // Formater la chaîne de sortie
    snprintf(buff, MSG_LEN, "connected since %s with IP address %s and port number %d", time_str, ip_str, current->port);


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
			break;
		current=current->next;
    }
	return 1;
}

int echo_send(int sockfd,int * num_clients,int i,struct pollfd * fds,enum msg_type type,char infos[INFOS_LEN],char buff[MSG_LEN],struct Client ** chaine_cli_head){

	//If message = "/quit"
	if (strcmp(buff, "/quit\n") == 0) {
		printf("Client requested to close the connection. Closing...\n");
		
		// Empty chaine
        if (chaine_cli_head==NULL){
            return 0;
        }
		removeClient(chaine_cli_head,sockfd);

		// Sending closing message 
		if (send_struct(sockfd,"",type,infos,buff)==0)
			return 0;

		printf("Closing message sent!\n");
		
		
		*num_clients=*num_clients-1;
		printf("Client disconnected. Total clients: %d\n", *num_clients);

		close(sockfd);
		fds[i].fd=0;
        fds[i].events=0;
        fds[i].revents=0;
		return 1;

	}

	return send_struct(sockfd,"",type,infos,buff);
}

int multicast_members(int sockfd, char nick_sender[NICK_LEN],char infos[INFOS_LEN],struct Client *chaine_cli_head) {
    struct Client *current = chaine_cli_head;

    // Liste des clients en ligne
    char buff[MSG_LEN];
    memset(buff, 0, MSG_LEN);
    
    while (current != NULL) {
		if (strcmp(infos, current->salon) == 0) {
			strcat(buff, current->nickname);
			strcat(buff, "\n");
		}
		current = current->next;
	}

    return send_struct(sockfd, "", MULTICAST_MEMBERS, infos, buff);
}

int multicast_list(int sockfd,struct Channel all_channel[MAX_CHANNEL]){
	char buff[MSG_LEN];
	memset(buff,0,MSG_LEN);


	//Find the channel
	for (int i_channel = 0; i_channel < MAX_CHANNEL; i_channel++){
		if (strlen(all_channel[i_channel].name) != 0 ) {
			strcat(buff,all_channel[i_channel].name );
			strcat(buff, "\n");
		}
	}

    return send_struct(sockfd, "", MULTICAST_LIST, "", buff);
}

int multicast_send(int sockfd,char nick_sender[NICK_LEN],char infos[INFOS_LEN],char buff[MSG_LEN],struct Client * chaine_cli_head){
	struct Client * current=chaine_cli_head;

	//Send to all connected clients in the channel except the sender.
	while (current!=NULL) {
		if (current->sockfd!=sockfd && (strcmp(current->salon, infos) == 0)){
			if (send_struct(current->sockfd,nick_sender,MULTICAST_SEND,infos,buff)==0)
				return 0;
		}
		current=current->next;
    }
	return 1;
}

int multicast_quit(int sockfd,char nick_sender[NICK_LEN],char infos[INFOS_LEN],struct Client * chaine_cli_head,struct Channel all_channel[MAX_CHANNEL]){
	//Update Client list
	//Find the client in the list
	struct Client* current = chaine_cli_head;
	while (current->next != NULL) {
		if (current->sockfd == sockfd) {
			break;
		}
		current = current->next;
    }

	//Client not found
	if (current==NULL){
		printf("Client not found");
		return 1;
	}

	//Modified Client information
	int i_channel=current->i_salon;
	current->i_salon=-1;
	free(current->salon);
	current->salon=malloc(NICK_LEN*sizeof(char));;

	//Update AllChannel list
	all_channel[i_channel].nbr_client-=1;

	//Last Client
	char buff[MSG_LEN];
	memset(buff,0,MSG_LEN);
	if (all_channel[i_channel].nbr_client==0){
		sprintf(buff, "You were the last user in this channel, %s has been destroyed", infos);
		free(all_channel[i_channel].name);
		all_channel[i_channel].name= malloc(NICK_LEN*sizeof(char));
		return send_struct(sockfd,"INFOS",MULTICAST_QUIT,"",buff) ;
	}


	else{
		//Inform of the deconnexion
		sprintf(buff, "%s quit the Channel", current->nickname);
		if (multicast_send(sockfd,"INFOS",infos,buff,chaine_cli_head)==0)
			return 0;
		return send_struct(sockfd,"INFOS",MULTICAST_QUIT,"","You are disconnected") ;
	}
}
	
int multicast_join(int sockfd,char nick_sender[NICK_LEN],char infos[INFOS_LEN],struct Client * chaine_cli_head,struct Channel all_channel[MAX_CHANNEL]){
	//Find the channel
	int i_channel;
	for ( i_channel = 0; i_channel < MAX_CHANNEL; i_channel++){
		if (strcmp(all_channel[i_channel].name, infos) == 0) 
            break;
		if (i_channel==MAX_CHANNEL-1){
			return send_struct(sockfd,"",ECHO_SEND,"","No channel found") ;
		}
	}

	//Find the client in the list
	struct Client* current = chaine_cli_head;
	while (current->next != NULL) {
		if (current->sockfd == sockfd) {
			break;
		}
		current = current->next;
    }

	//Client not found
	if (current==NULL){
		printf("Client not found");
		return 1;
	}

	//Client already in a channel
	if (current->i_salon!=-1){
		if (multicast_quit(sockfd,nick_sender,infos,chaine_cli_head,all_channel)==0){
			return 0;
		}
	}
	
	//Update Client List
	current->i_salon=i_channel;
	strcpy(current->salon, infos);

	//Uptdate Channel List
	all_channel[i_channel].nbr_client+=1;

	//Inform people
	char buff[MSG_LEN];
	memset(buff,0,MSG_LEN);
	sprintf(buff, "%s join the Channel", nick_sender);
	if (multicast_send(sockfd,"INFOS",infos,buff,chaine_cli_head)==0)
		return 0;
	sprintf(buff, "You have joined %s ", infos);
	return send_struct(sockfd,"INFOS",MULTICAST_JOIN,infos,buff) ;
}

int multicast_create(int sockfd,char nick_sender[NICK_LEN],char infos[INFOS_LEN],struct Client * chaine_cli_head,struct Channel all_channel[MAX_CHANNEL]){
	char buff[MSG_LEN];
	memset(buff,0,MSG_LEN);

	//Existing channel
	if (is_existing_channel(all_channel,infos)==1){
		strcpy(buff,"Channel already exists");
		return send_struct(sockfd,"INFOS",MULTICAST_CREATE,"",buff);
	}
	

	//Find free channel
	int i_new_channel;
	for (i_new_channel = 0; i_new_channel < MAX_CHANNEL; i_new_channel++){
		//Free channel found
		if (all_channel[i_new_channel].nbr_client==0){
			strcpy(all_channel[i_new_channel].name,infos);
			break;
		}

		//No free channels
		if (i_new_channel==MAX_CHANNEL-1){
			strcpy(buff,"No free channels");
			return send_struct(sockfd,"INFOS",MULTICAST_CREATE,"",buff);
		}	
	}

	return multicast_join(sockfd,nick_sender,infos,chaine_cli_head,all_channel);

}

int file_request(int sockfd,char nick_sender[NICK_LEN],char infos[INFOS_LEN],char buff[MSG_LEN],struct Client * chaine_cli_head){
	struct Client * current=chaine_cli_head;

	//Send to user 
	while (current!=NULL) {
		if (strcmp(current->nickname, infos) == 0){
			return send_struct(current->sockfd,nick_sender,FILE_REQUEST,infos,buff) ;
		}	
		current=current->next;
    }
	return 1;
}

int file_reject(int sockfd,char nick_sender[NICK_LEN],char infos[INFOS_LEN],struct Client * chaine_cli_head){
	struct Client * current=chaine_cli_head;

	//Send to user 
	while (current!=NULL) {
		if (strcmp(current->nickname, infos) == 0){
			return send_struct(current->sockfd,nick_sender,FILE_REJECT,infos,"") ;
		}
		current=current->next;
    }
	return 1;
}

int file_accept(int sockfd,char nick_sender[NICK_LEN],char infos[INFOS_LEN], char buff[MSG_LEN],struct Client * chaine_cli_head){
	struct Client * current=chaine_cli_head;

	//Send to user 
	while (current!=NULL) {
		if (strcmp(current->nickname, infos) == 0){
			return send_struct(current->sockfd,nick_sender,FILE_ACCEPT,infos,buff) ;
		}
		current=current->next;
    }
	return 1;
}

int file_ack(int sockfd,char nick_sender[NICK_LEN],char infos[INFOS_LEN],struct Client * chaine_cli_head){
	struct Client * current=chaine_cli_head;

	//Send to user 
	while (current!=NULL) {
		if (strcmp(current->nickname, infos) == 0)
			return send_struct(current->sockfd,nick_sender,FILE_ACK,infos,"") ;
			break;
		current=current->next;
    }
	return 1;
}

int echo_server(int* num_clients,struct pollfd * fds,int i,struct Client ** chaine_cli_head,struct Channel all_channel[MAX_CHANNEL]) {
	int sockfd=fds[i].fd;
	
	struct message msgstruct;
	char buff[MSG_LEN];

	// Cleaning memory
	memset(&msgstruct, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN);

	// Receiving structure
	size_t totalStructBytesReceived = 0;
	while (totalStructBytesReceived < sizeof(struct message)) {
		ssize_t structBytesReceived = recv(sockfd, ((char*)&msgstruct) + totalStructBytesReceived, sizeof(struct message) - totalStructBytesReceived, 0);
		if (structBytesReceived <= 0) {
			perror("Erreur lors de la réception de la structure");
			return 0;  // Ou prendre d'autres mesures en cas d'échec de réception
		}
		totalStructBytesReceived += structBytesReceived;
	}

	// Receiving message
	size_t totalMsgBytesReceived = 0;
	while (totalMsgBytesReceived < msgstruct.pld_len) {
		ssize_t msgBytesReceived = recv(sockfd, buff + totalMsgBytesReceived, msgstruct.pld_len - totalMsgBytesReceived, 0);
		if (msgBytesReceived <= 0) {
			perror("Erreur lors de la réception du champ buff");
			return 0;  // Ou prendre d'autres mesures en cas d'échec de réception
		}
		totalMsgBytesReceived += msgBytesReceived;
	}


	printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
	
	//ECHO_SEND
	if (msgstruct.type == ECHO_SEND)
		return echo_send(sockfd,num_clients,i,fds,msgstruct.type,msgstruct.infos,buff,chaine_cli_head);

	//NICKNAME_NEW
	if (msgstruct.type == NICKNAME_NEW)
		return nickname_new(sockfd,msgstruct.nick_sender,msgstruct.infos,*chaine_cli_head);

	//NICKNAME_LIST
	if (msgstruct.type == NICKNAME_LIST)
		return nickname_list(sockfd,*chaine_cli_head);
	
	//NICKNAME_INFOS
	if (msgstruct.type == NICKNAME_INFOS)
		return nickname_infos(sockfd,msgstruct.infos,*chaine_cli_head);
	
	//The client has no nickname yet
	if (strcmp(msgstruct.nick_sender, "\0") == 0){
		char buff[MSG_LEN]="please login with /nick <your pseudo>";
		return send_struct(sockfd,"\0",NICKNAME_NEW,"/0",buff);
	}

	//BROADCAST_SEND
	if (msgstruct.type == BROADCAST_SEND)
		return broadcast_send(sockfd,msgstruct.nick_sender,msgstruct.type,msgstruct.infos,buff,*chaine_cli_head);
	
	//UNICAST_SEND
	if (msgstruct.type == UNICAST_SEND)
		return unicast_send(sockfd,msgstruct.nick_sender,msgstruct.type,msgstruct.infos,buff,*chaine_cli_head);

	//MULTICAST_CREATE
	if (msgstruct.type == MULTICAST_CREATE)
		return multicast_create(sockfd,msgstruct.nick_sender,msgstruct.infos,*chaine_cli_head,all_channel);
	
	//MULTICAST_MEMBERS
	if (msgstruct.type == MULTICAST_MEMBERS)
		return multicast_members(sockfd,msgstruct.nick_sender,msgstruct.infos,*chaine_cli_head);

	//MULTICAST_LIST
	if (msgstruct.type == MULTICAST_LIST)
		return multicast_list(sockfd,all_channel);

	//MULTICAST_JOIN
	if (msgstruct.type == MULTICAST_JOIN)
		return multicast_join(sockfd,msgstruct.nick_sender,msgstruct.infos,*chaine_cli_head,all_channel);
	
	//MULTICAST_SEND
	if (msgstruct.type == MULTICAST_SEND)
		return multicast_send(sockfd,msgstruct.nick_sender,msgstruct.infos,buff,*chaine_cli_head);

	//MULTICAST_QUIT
	if (msgstruct.type == MULTICAST_QUIT)
		return multicast_quit(sockfd,msgstruct.nick_sender,msgstruct.infos,*chaine_cli_head,all_channel);
	
	//FILE_REQUEST
	if (msgstruct.type == FILE_REQUEST)
		return file_request(sockfd,msgstruct.nick_sender,msgstruct.infos,buff,*chaine_cli_head);

	//FILE_REJECT
	if (msgstruct.type == FILE_REJECT)
		return file_reject(sockfd,msgstruct.nick_sender,msgstruct.infos,*chaine_cli_head);

	//FILE_ACCEPT
	if (msgstruct.type == FILE_ACCEPT)
		return file_accept(sockfd,msgstruct.nick_sender,msgstruct.infos,buff,*chaine_cli_head);
	
	//FILE_ACK
	if (msgstruct.type == FILE_ACK)
		return file_ack(sockfd,msgstruct.nick_sender,msgstruct.infos,*chaine_cli_head);

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

	struct Client * chaine_cli_head = NULL;

	struct Channel all_channel[MAX_CHANNEL];
	for (int i = 0; i < MAX_CHANNEL; i++) {
        all_channel[i].nbr_client = 0;  
		all_channel[i].name = malloc(NICK_LEN*sizeof(char));     
    }
	
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
				struct Client *new_client=(struct Client*)malloc(sizeof(struct Client));
				if (new_client == NULL) {
					fprintf(stderr, "Erreur d'allocation mémoire pour le client.\n");
					return EXIT_FAILURE;
				}
				new_client->sockfd=new_fd;
				new_client->addr=client_addr.sin_addr;
				new_client->port=client_addr.sin_port;
				new_client->connectionTime = time(NULL);
				new_client->nickname=malloc(NICK_LEN*sizeof(char));
				new_client->salon=malloc(NICK_LEN*sizeof(char));
				new_client->i_salon=-1;
                new_client->next=NULL;

				char buff[MSG_LEN]="please login with /nick <your pseudo>";
				if(send_struct(new_fd,"\0",NICKNAME_NEW,"\0",buff)==0)
					exit(EXIT_FAILURE);

				//First Client
				if (chaine_cli_head == NULL) {
					chaine_cli_head = new_client;
					
				} 
				else {
					struct Client *current = chaine_cli_head;
					while (current->next != NULL) {
						current = current->next;
					}
					current->next = new_client;
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
				int ret=echo_server(&num_clients,fds,i,&chaine_cli_head,all_channel)  ;
				if (ret==0){
                    perror("echo_server");
                    exit(EXIT_FAILURE);
                }
				fds[i].revents=0;
            }
        }
    }

    close(sfd);
    return EXIT_SUCCESS;
}











