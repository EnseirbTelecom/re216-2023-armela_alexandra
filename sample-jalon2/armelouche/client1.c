#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include "common.h"
#include "msg_struct.h"


#define MAX_EVENTS 2

// MAX_EVENTS 2 : 

void echo_client(int sockfd) {

    struct message msgstruct;
   

    char buff[MSG_LEN];
    int n;
    
    // Initialisation de poll
    struct pollfd fds[MAX_EVENTS];
    // reset la mémoire
    memset(fds, 0, MAX_EVENTS * sizeof(struct pollfd));

    // socket d'écoute : clavier 
    fds[0].fd = STDIN_FILENO; 
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    // Socket d'écoute: connection 
    fds[1].fd = sockfd; 
    fds[1].events = POLLIN;
    fds[1].revents = 0;


    while (1) {

        memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
        
        // Getting message from client 
        printf("Message: \n");
        
        int active_poll = poll(fds, MAX_EVENTS, -1);
        if (active_poll < 0) {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        
        for(int i=0; i<MAX_EVENTS; i++){
            if (fds[i].revents & (POLLIN | POLLHUP)){
                
                // If there is data available from the keyboard (stdin).
                if (fds[i].fd == STDIN_FILENO) {
                    memset(buff, 0, MSG_LEN);
                    n = 0;
                    while ((buff[n++] = getchar()) != '\n') {}
                    //Filling structure
                    msgstruct.type = ECHO_SEND;
                    msgstruct.pld_len = strlen(buff);

                    // Sending struct
                    if (send(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
                        break;
                    }
                    //Sending message
                    if (send(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
			            break;
		            }
                    printf("Message sent: %s\n", buff);
                    //Cleaning memory 
                    memset(&msgstruct, 0, sizeof(struct message));
					memset(buff, 0, MSG_LEN);
                    fds[i].events = POLLIN; 
					fds[i].revents = 0;
                }

                // If data is available from the server.
                else if (fds[i].fd == sockfd) {
                    // Receiving structure
                    if (recv(sockfd, &msgstruct, sizeof(struct message), 0) <= 0) {
			            break;
		            }
                    // Receiving message
		            if (recv(sockfd, buff, msgstruct.pld_len, 0) <= 0) {
                        break;
		            }

                    if (strcmp(buff, "/quit\n") == 0) {
                        close(sockfd);
                        printf("Connection fermé par le client\n");
                        exit(EXIT_SUCCESS);  // Arrête le programme client.
                    }
                    printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);
		            printf("Received: %s", buff);   
                }
            }   
            // si la connexion terminé on enlève du tableau 
            if (fds[i].revents & POLLHUP) {
                close(fds[i].fd);
                fds[i].fd = 0;
            }
        }
    }
}




int handle_connect(char *server_name, char *server_port) {
    struct addrinfo hints, *result, *rp;
    int sfd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server_name, server_port, &hints, &result) != 0) {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
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

    char *server_name = argv[1];
    char *server_port = argv[2];

    int sfd = handle_connect(server_name, server_port);

    echo_client(sfd);
    return EXIT_SUCCESS;
}
