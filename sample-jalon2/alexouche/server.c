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
#include "msg_struct.h"

struct Client {
    int sockfd;
	char* nickname;
    struct in_addr addr;
	unsigned short int port;
    struct Client * next;
};

int ask_nickname(char * my_nickname,int sockfd){
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

int echo_server(int* num_clients,struct pollfd * fds,int i,char **nick_client,char** clients_connected,struct Client * chaine_cli_head) {
	struct message msgstruct;
	char buff[MSG_LEN];

	int sockfd=fds[i].fd;
	

	// Cleaning memory
	memset(&msgstruct, 0, sizeof(struct message));
	memset(buff, 0, MSG_LEN);

	// Receiving structure
	if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
		perror("recv");
		printf("Client disconnected. Total clients: %d\n", *num_clients);
		close(sockfd);
		fds[i].fd=0;
		return(0);
	}
	// Receiving message
	if (recv(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
		perror("recv");
		return 0;
	}

	


	if (msgstruct.type ==NICKNAME_NEW )
	{
		printf("Message type nickname\n");
		for (int i = 0; i < 128; i++) {
			if (strcmp(msgstruct.infos, clients_connected[i]) == 0) {
				int ret = nickname_already_attribuate("server",sockfd);
				if (ret==0){
					return 0;
				}
				return 1; // La chaîne est trouvée dans le tableau
			}
		}

		// On met à jour dans la liste des clients connectés
		*nick_client=msgstruct.infos;
		printf("Nickname updated in connected client list\n");
	
		
		
		//OPn met à jour dans la liste des clients
		struct Client * find_cli = chaine_cli_head;
			
		while (find_cli->next!=NULL){
			if ((find_cli->nickname==msgstruct.nick_sender)&&(find_cli->sockfd==sockfd)){
				find_cli->nickname=msgstruct.infos;
				printf("Nickname updated in client linked list\n");
				break;
			}
			else 
				find_cli=find_cli->next;
		}
	}
	
	printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
	printf("Received: %s", buff);
	printf(" hello %s \n",msg_type_str[msgstruct.type]);
	// Sending structure (ECHO)
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
    int max_conn = 256;
    struct pollfd fds[max_conn];
    memset(fds, 0, max_conn * sizeof(struct pollfd));

    // Insertion de listen_fd dans le tableau
    fds[0].fd = sfd;
    fds[0].events = POLLIN;

	char my_nickname[NICK_LEN]="server";
	char **clients_connected=malloc(max_conn*sizeof(int*));
	for (int i = 0; i < max_conn; i++)
	{
		clients_connected[i]=malloc(NICK_LEN*sizeof(int));
	}
	

	struct Client * chaine_cli = NULL;
	struct Client * chaine_cli_head = NULL;
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
				new_client->nickname="/0";
                new_client->next=NULL;

				ask_nickname(my_nickname,new_fd);

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
				if (strcmp(clients_connected[i],"/0"))
				{
					ask_nickname(my_nickname,fds[i].fd);
				}
				
                int ret=echo_server(&num_clients,fds,i,&clients_connected[i],clients_connected,chaine_cli_head)  ;
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














