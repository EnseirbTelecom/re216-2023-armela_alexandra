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
#include <assert.h>
#include <sys/wait.h>


#include "common.h"
#include "msg_struct.h"

// AUXILIARY FUNCTIONS
int containsOnlyAlphanumeric(char str[]) {
    for (int i = 0; i < strlen(str); i++){
		if(!isalpha(str[i])&&!isdigit(str[i]))
			return 0;
	}
    return 1; 
}

// SENDING A STRUCT
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
	
	printf("\nMessage sent!\n");

	return 1;
}

// FUNCTIONS FOR KEYBOARD INPUT PROCESSING
// USERS
int nick(char buff[MSG_LEN],int sockfd_server,char my_nickname[NICK_LEN]){
	char *infos = strtok(buff, " ");
	
	if (infos== NULL){
		printf("[Warning] : No space between /nick and <your pseudo> \n");
		return 1;
	}

	// Get the new nickname
	infos = strtok(NULL, "");
	
	if (infos== NULL){
		printf("[Warning] : No nickname\n");
		return 1;
	}

	char new_nickname[NICK_LEN];
	memset(new_nickname, 0, NICK_LEN);
	strncpy(new_nickname, infos, strlen(infos)-1);
	int len_nickname=strlen(new_nickname);

	if ((len_nickname<3)||(len_nickname>127)){
		printf("[Warning] : The new nickname size is incorrect; it should be between 3 and 127 characters.\n");
		return 1;
	}

	if (containsOnlyAlphanumeric(new_nickname)==0){
		printf("[Warning] : The new nickname should only contain letters of the alphabet or numbers.\n");
		return 1;
	}

	char empty[MSG_LEN];
	memset(empty,0,MSG_LEN);
	return send_struct(sockfd_server,my_nickname,NICKNAME_NEW,new_nickname,empty);
		
}

int who(int sockfd_server,char my_nickname[NICK_LEN]){
	char empty[MSG_LEN];
	memset(empty,0,MSG_LEN);
	if(send_struct(sockfd_server,my_nickname,NICKNAME_LIST,empty,empty)==0)
		return 0;
			
	return 1;
}

int whois(char buff[MSG_LEN],int sockfd_server,char my_nickname[NICK_LEN]){
	char *infos = strtok(buff, " ");

	if (infos== NULL){
		printf("[Warning] : No space between /whois and <pseudo> \n");
		return 1;
	}

	// Get the nickname
	infos = strtok(NULL, "");

	if (infos== NULL){
		printf("[Warning] : No nickname\n");
		return 1;
	}

	char nickname[NICK_LEN];
	memset(nickname, 0, NICK_LEN);
	strncpy(nickname, infos, strlen(infos)-1);
	int len_nickname=strlen(nickname);

	
	if ((len_nickname<3)||(len_nickname>127)){
		printf("[Warning] : The nickname size is incorrect; it should be between 3 and 127 characters.\n");
		return 1;
	}

	if (containsOnlyAlphanumeric(nickname)==0){
		printf("[Warning] : The nickname should only contain letters of the alphabet or numbers.\n");
		return 1;
	}
	char empty[MSG_LEN];
	memset(empty,0,MSG_LEN);
	return send_struct(sockfd_server,my_nickname,NICKNAME_INFOS,nickname,empty);
		
}

int msgall(char buff[MSG_LEN],int sockfd_server,char my_nickname[NICK_LEN]){
	char *infos = strtok(buff, " ");

	if (infos== NULL){
		printf("[Warning] : No space between /msgall and <message> \n");
		return 1;
	}

	// Get the message
	infos = strtok(NULL, "");
	char* msg=infos;
	char empty[MSG_LEN];
	memset(empty,0,MSG_LEN);
	return send_struct(sockfd_server,my_nickname,BROADCAST_SEND,empty,msg);
		
}

int msg(char buff[MSG_LEN],int sockfd_server,char my_nickname[NICK_LEN]){
	char *infos = strtok(buff, " ");

	if (infos== NULL){
		printf("[Warning] : No space between <pseudo> and <message> \n");
		return 1;
	}

	// Get the nickname
	infos = strtok(NULL, " ");

	if (infos== NULL){
		printf("[Warning] : No nickname\n");
		return 1;
	}

	char nickname[NICK_LEN];
	memset(nickname, 0, NICK_LEN);
	strncpy(nickname, infos, strlen(infos));
	int len_nickname=strlen(nickname);

	// Nickname verification (size/alphanumeric)
	if ((len_nickname<3)||(len_nickname>127)){
		printf("[Warning] : The nickname size is incorrect; it should be between 3 and 127 characters.");
		return 1;
	}

	if (containsOnlyAlphanumeric(nickname)==0){
		printf("[Warning] : The nickname should only contain letters of the alphabet or numbers.");
		return 1;
	}

	infos = strtok(NULL, "");

	if (infos== NULL){
		printf("[Warning] : No message\n");
		return 1;
	}

	char* msg=infos;
	return send_struct(sockfd_server,my_nickname,UNICAST_SEND,nickname,msg);
}

// CHAT ROOM
int create(char buff[MSG_LEN],int sockfd_server,char my_nickname[NICK_LEN], char* my_salon){
	char *infos = strtok(buff, " ");

	if (infos== NULL){
		printf("[Warning] : No space between <create> and <chanel_name> \n");
		return 1;
	}

	// Get channel name 
	infos = strtok(NULL, " ");

	if (infos== NULL){
		printf("[Warning] : No chanel_name\n");
		return 1;
	}
 
	char new_chanel_name[NICK_LEN];
	memset(new_chanel_name, 0, NICK_LEN);
	strncpy(new_chanel_name, infos, strlen(infos)-1);
	int len_new_chanel_name =  strlen(new_chanel_name);

	if ((len_new_chanel_name<3)||(len_new_chanel_name>127)){
		printf("[Warning] : The new chanel name size is incorrect; it should be between 3 and 127 characters.");
		return 1;
	}

	if (containsOnlyAlphanumeric(new_chanel_name)==0){
		printf("[Warning] : The new chanel name should only contain letters of the alphabet or numbers.");
		return 1;
	}

	char empty[MSG_LEN];
	memset(empty,0,MSG_LEN);

	if (strlen(my_salon)!=0){
		if (send_struct(sockfd_server,my_nickname,MULTICAST_QUIT,my_salon,empty)==0)
			return 0;	
	}
	
	return send_struct(sockfd_server,my_nickname,MULTICAST_CREATE,new_chanel_name,empty);
	
}

int list(char buff[MSG_LEN], int sockfd_server, char my_nickname[NICK_LEN]){
	char empty[MSG_LEN];
	memset(empty,0,MSG_LEN);

	if(send_struct(sockfd_server,my_nickname,MULTICAST_LIST,empty,empty)==0)
		return 0;
			
	return 1;
}

int channel_members(char buff[MSG_LEN], int sockfd_server, char my_nickname[NICK_LEN]){
	char *infos = strtok(buff, " ");

	if (infos== NULL){
		printf("[Warning] : No space between /channel_members and <channel_name> \n");
		return 1;
	}

	// Get channel name 
	infos = strtok(NULL, " ");

	if (infos== NULL){
		printf("[Warning] : No chanel name\n");
		return 1;
	}

	char chanel_name[NICK_LEN];
	memset(chanel_name,0,NICK_LEN);
	strncpy(chanel_name, infos, strlen(infos)-1);
	int len_chanel_name =  strlen(chanel_name);

	if ((len_chanel_name<3)||(len_chanel_name>127)){
		printf("[Warning] : The new chanel name size is incorrect; it should be between 3 and 127 characters.");
		return 1;
	}

	if (containsOnlyAlphanumeric(chanel_name)==0){
		printf("[Warning] : The new chanel name should only contain letters of the alphabet or numbers.");
		return 1;
	}
	char empty[MSG_LEN];
	memset(empty,0,MSG_LEN);

	return send_struct(sockfd_server,my_nickname,MULTICAST_MEMBERS,chanel_name,empty);
	
}

int join(char buff[MSG_LEN], int sockfd_server, char my_nickname[NICK_LEN],char* my_salon){
	char *infos = strtok(buff, " ");

	if (infos== NULL){
		printf("[Warning] : No space between /join and <channel_name> \n");
		return 1;
	}

	// Get channel name 
	infos = strtok(NULL, " ");

	if (infos== NULL){
		printf("[Warning] : No channel name\n");
		return 1;
	}

	char new_chanel_name[NICK_LEN];
	memset(new_chanel_name,0,NICK_LEN);
	strncpy(new_chanel_name, infos, strlen(infos)-1);
	int len_new_chanel_name =  strlen(new_chanel_name);

	if ((len_new_chanel_name<3)||(len_new_chanel_name>127)){
		printf("[Warning] : The channel name size is incorrect; it should be between 3 and 127 characters.");
		return 1;
	}

	if (containsOnlyAlphanumeric(new_chanel_name)==0){
		printf("[Warning] : The channel name should only contain letters of the alphabet or numbers.");
		return 1;
	}

	char empty[MSG_LEN];
	memset(empty,0,MSG_LEN);

	if (strlen(my_salon)!=0){
		if (send_struct(sockfd_server,my_nickname,MULTICAST_QUIT,my_salon,empty)==0)
			return 0;	
	}

	return send_struct(sockfd_server,my_nickname,MULTICAST_JOIN,new_chanel_name,empty);
	
}

int quit(char buff[MSG_LEN], int sockfd_server, char my_nickname[NICK_LEN], char my_salon[NICK_LEN]){
	char *infos = strtok(buff, " ");

	if (infos== NULL){
		printf("[Warning] : No space between /quit and <channel_name> \n");
		return 1;
	}

	//Get channel name 
	infos = strtok(NULL, " ");

	if (infos== NULL){
		printf("[Warning] : No chanel name input\n");
		return 1;
	}

	//Check right channel name 
	if (strncmp(infos, my_salon, strlen(my_salon))!=0){
		printf("[Warning] : You are not in this channel\n");
		return 1;
	}
	 
	char chanel_name_quited[NICK_LEN];
	memset(chanel_name_quited,0,NICK_LEN);
	strncpy(chanel_name_quited, infos, strlen(infos)-1);
	int len_chanel_name_quited =  strlen(chanel_name_quited);

	if ((len_chanel_name_quited<3)||(len_chanel_name_quited>127)){
		printf("[Warning] : The new chanel name size is incorrect; it should be between 3 and 127 characters.");
		return 1;
	}

	if (containsOnlyAlphanumeric(chanel_name_quited)==0){
		printf("[Warning] : The new chanel name should only contain letters of the alphabet or numbers.");
		return 1;
	}

	char empty[MSG_LEN];
	memset(empty,0,MSG_LEN);

	return send_struct(sockfd_server,my_nickname,MULTICAST_QUIT,chanel_name_quited,empty);
	
}

// FILE TRANSFERT
int file_request(char buff[MSG_LEN],int sockfd_server,char my_nickname[NICK_LEN],char * filename){
	char *infos = strtok(buff, " ");

	if (infos== NULL){
		printf("[Warning] : No space between <username> and <file_path> \n");
		return 1;
	}

	//Get the username
	infos = strtok(NULL, " ");

	if (infos== NULL){
		printf("[Warning] : No username\n");
		return 1;
	}

	char username[NICK_LEN];
	memset(username,0,NICK_LEN);
	strncpy(username, infos, strlen(infos));
	int len_username=strlen(username);

	//Client want to send a file to himself
	if (strcmp(my_nickname, username)==0){
		printf("[Warning] : Sending a file to himself is not possible.\n");
		return 1;
	}

	//Nickname verification (size/alphanumeric)
	if ((len_username<3)||(len_username>127)){
		printf("[Warning] : The username size is incorrect; it should be between 3 and 127 characters.\n");
		return 1;
	}

	if (containsOnlyAlphanumeric(username)==0){
		printf("[Warning] : The username should only contain letters of the alphabet or numbers.\n");
		return 1;
	}

	infos = strtok(NULL, "");

	if (infos== NULL){
		printf("[Warning] : No file_path\n");
		return 1;
	}

	strncpy(filename,infos, strlen(infos)-1);

    char *lastSlash = strrchr(infos, '/');
    // If the '/' character is found, advance one position to obtain the file name
    if (lastSlash != NULL) {
        lastSlash++;  
    } else {
        lastSlash = infos;
    }

	char file_path[NICK_LEN];
	memset(file_path,0,NICK_LEN);
	strncpy(file_path, lastSlash, strlen(lastSlash)-1);
	int len_file_path=strlen(file_path);

	// File_path verification (size/existance)
	if ((len_file_path<3)||(len_file_path>127)){
		printf("[Warning] : The file_path size is incorrect; it should be between 3 and 127 characters.");
		return 1;
	}

	if (access(filename, F_OK) == -1) {
        printf("[Warning] : File doesn't exist.\n");
		return 1;
    } 

	char f[MSG_LEN];
	memset(f,0,strlen(f));
	strncpy(f,file_path,strlen(file_path));

	return send_struct(sockfd_server,my_nickname,FILE_REQUEST,username,f);
}

int file_accept(char nickname_recv[NICK_LEN],char buff[MSG_LEN],char my_nickname[NICK_LEN],char* file_path){
	printf("Connecting to %s and sending the file...\n", nickname_recv);

	char *infos = strtok(buff, ":");

	// Get addr
	if (infos== NULL){
		printf("[Warning] : buff not addr:port \n");
		return 1;
	}

	char ipaddr[9];
	memset(ipaddr, 0, 9);
	strncpy(ipaddr, infos, 9);

	// Get port
	infos = strtok(NULL, "");

	if (infos== NULL){
		printf("[Warning] : No port\n");
		return 1;
	}

	char port[strlen(infos)];
	memset(port, 0, strlen(infos));
	strncpy(port, infos, strlen(infos));
	printf("fichier : %s  %lu \n", file_path,strlen(file_path));

	// Create child processes
	char*args[6]={"sender",ipaddr,port,my_nickname,file_path,NULL};
	int fils=-1;

    fils=fork();
    
    if (fils==0){
        execv("./sender",args);
    }
    else if (fils>0){
        int child2=-1;
        child2=wait(NULL);
        assert(fils==child2);
    }
    printf("\n [Info] : File sending complete \n");
	return 1;			
}

int send_answer(int sockfd_server,struct pollfd fds[CONN_CLI],char my_nickname[NICK_LEN],char username[NICK_LEN]){
	char empty[MSG_LEN];
	memset(empty,0,MSG_LEN);

	while (1) {
        // Call for poll in anticipation of new events
        int active_fds = poll(fds, CONN_CLI, -1);
		if (active_fds<0){
			perror("poll");
            exit(EXIT_FAILURE);
		}
		
		// Activity on standard input socket
		if ((fds[1].revents & POLLIN) == POLLIN) {
			char buff[MSG_LEN];
			// Cleaning memory
			memset(buff, 0, MSG_LEN);
			// Getting message from client
			int n = 0;
			while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent

			// Client accepts the file transfert
			if (strncmp(buff, "Y", strlen("Y"))==0){
				char buff2[MSG_LEN];
				// Cleaning memory
				memset(buff2, 0, MSG_LEN);
				sprintf(buff2, "%s:%s",FILE_ADDR, FILE_PORT );

				fds[1].revents=0;
				if (send_struct(sockfd_server,my_nickname,FILE_ACCEPT,username,buff2)==0)
					return 0;

				// Create child processes
				char*args[3]={"receiver",FILE_PORT,NULL};
				int fils=-1;
				fils=fork();
				
				if (fils==0){
					execv("./receiver",args);

				}
				else if (fils>0){
					int child2=-1;
					child2=wait(NULL);
					assert(fils==child2);

				}
				
				printf("\n [INFOS] : File reception complete \n");
				return send_struct(sockfd_server,my_nickname,FILE_ACK,username,empty);		
			}
			
			// Client refuses the file transfert
			if (strncmp(buff, "N", strlen("N"))==0){
				fds[1].revents=0;
				return send_struct(sockfd_server,my_nickname,FILE_REJECT,username,empty);
			}
			printf("[Warning] : Do you accept? [Y/N]\n");
		}
	}
}

// BONUS
int help() {
    printf("Available commands:\n");
    printf("/nick <your_pseudo> - Set your nickname\n");
    printf("/who - List online users\n");
    printf("/whois <username> - Display information about a specific user\n");
    printf("/msgall <message> - Send a message to all online users\n");
    printf("/msg <username> <message> - Send a private message to a specific user\n");
    printf("/create <channel_name> - Create a new channel\n");
	printf("/channel_list - List of channels\n");
	printf("/channel_members <channel_name> - List of channel member\n");
    printf("/join <channel_name> - Join an existing channel\n");
    printf("/quit <channel_name> - Quit a channel\n");
    printf("/send <username> <file_path> - Send a file to a specific user\n");
    printf("/help - Display this help message\n");
	printf("/kill_server - Stop the chat\n\n");
	return 1;
}

// ANALYSE THE ACTIVITY
int echo_client(struct pollfd fds[CONN_CLI],int sockfd_active,int sockfd_server, int sockfd_entree,char my_nickname[NICK_LEN], char my_salon[NICK_LEN],char* filename) {
	struct message msgstruct;
	char buff[MSG_LEN];
	int n;
	char empty[MSG_LEN];
	memset(empty,0,MSG_LEN);

	// KEYBOARD INFORMATION PROCESSING
	if (sockfd_active==sockfd_entree)
	{
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
		memset(buff, 0, MSG_LEN);
		// Getting message from client
		n = 0;
		while ((buff[n++] = getchar()) != '\n') {} // trailing '\n' will be sent

		//KILL_SERVER
		if (strncmp(buff, "/kill", strlen("/kill"))==0)
			return send_struct(sockfd_server,my_nickname,KILL_SERVER,empty,empty);		
		
		//HELP
		if (strncmp(buff, "/help", strlen("/help"))==0)
			return help();		
		
		//NICKNAME_NEW
		if (strncmp(buff, "/nick ", strlen("/nick "))==0)
			return nick(buff,sockfd_server,my_nickname);		
			
		//NICKNAME_IN
		if (strncmp(buff, "/whois ", strlen("/whois "))==0)
			return whois(buff,sockfd_server,my_nickname);

		//NICKNAME_LIST
		if (strncmp(buff, "/who", strlen("/who"))==0)
			return who(sockfd_server,my_nickname);
		
		//UNICAST_SEND
		if (strncmp(buff, "/msgall ", strlen("/msgall "))==0)
			return msgall(buff,sockfd_server,my_nickname);
		
		//BROADCAST_SEND
		if (strncmp(buff, "/msg ", strlen("/msg "))==0)
			return msg(buff,sockfd_server,my_nickname);

		//MULTICAST_CREATE 
		if (strncmp(buff, "/create ", strlen("/create "))==0)
			return create(buff,sockfd_server,my_nickname,my_salon);

		//MULTICAST_LIST
		if (strncmp(buff, "/channel_list", strlen("/channel_list"))==0)
			return list(buff,sockfd_server,my_nickname);

		//MULTICAST_MEMBERS
		if (strncmp(buff, "/channel_members", strlen("/channel_members"))==0)
			return channel_members(buff,sockfd_server,my_nickname);
		
		//MULTICAST_JOIN
		if (strncmp(buff, "/join ", strlen("/join "))==0)
			return join(buff,sockfd_server,my_nickname,my_salon);

		//FILE_REQUEST
		if (strncmp(buff, "/send ", strlen("/send "))==0)
			return file_request(buff,sockfd_server,my_nickname,filename);

		//ECHO_SEND
		if (strlen(my_salon) == 0)
			return send_struct(sockfd_server,my_nickname,ECHO_SEND,empty,buff);

		//MULTICAST_QUIT
		if (strncmp(buff, "/quit", strlen("/quit"))==0)
			return quit(buff,sockfd_server,my_nickname,my_salon);

		//MULTICAST_SEND
		return send_struct(sockfd_server,my_nickname,MULTICAST_SEND,my_salon,buff);

	}

	//PROCESSING INFORMATION RECEIVED FROM THE SERVER
	if (sockfd_active==sockfd_server)
	{
		// Cleaning memory
		memset(&msgstruct, 0, sizeof(struct message));
		size_t totalStructBytesReceived = 0;
		while (totalStructBytesReceived < sizeof(struct message)) {
			ssize_t structBytesReceived = recv(sockfd_server, ((char*)&msgstruct) + totalStructBytesReceived, sizeof(struct message) - totalStructBytesReceived, 0);
			if (structBytesReceived <= 0) {
				perror("Erreur lors de la réception de la structure");
				return 0; 
			}
			totalStructBytesReceived += structBytesReceived;
		}

		// Cleaning memory
		memset(buff, 0, MSG_LEN);

		// Receiving message
		size_t totalMsgBytesReceived = 0;
		while (totalMsgBytesReceived < msgstruct.pld_len) {
			ssize_t msgBytesReceived = recv(sockfd_server, buff + totalMsgBytesReceived, msgstruct.pld_len - totalMsgBytesReceived, 0);
			if (msgBytesReceived <= 0) {
				perror("Erreur lors de la réception du champ buff");
				return 0; 
			}
			totalMsgBytesReceived += msgBytesReceived;
		}

		//Show the struct
		//printf("pld_len: %i / nick_sender: %s / type: %s / infos: %s\n\n ", msgstruct.pld_len, msgstruct.nick_sender, msg_type_str[msgstruct.type], msgstruct.infos);

		//NICKNAME_NEW
		if (msgstruct.type == NICKNAME_NEW){
			printf("[Server]: %s \n", buff);
			memset(my_nickname,0,strlen(my_nickname));
			strncpy(my_nickname, msgstruct.infos, strlen(msgstruct.infos));
			return 1;
		}

		//NICKNAME_LIST
		if (msgstruct.type == NICKNAME_LIST){
			printf("[Server]: Online users are : \n%s\n",buff);
			return 1;
		}

		//NICKNAME_INFOS
		if (msgstruct.type ==NICKNAME_INFOS){
			printf("[Server]: %s %s\n",msgstruct.infos,buff);
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
            	printf("Server close the connection. Closing...\n");
            	close(sockfd_server);  // Close the socket.
            	exit(EXIT_SUCCESS);  
       		}
			return 1;
		}

		//MULTICAST_CREATE
		if (msgstruct.type ==MULTICAST_CREATE){
			printf("[Server]: %s \n", buff);
			return 1;
		}

		//MULTICAST_JOIN
		if (msgstruct.type == MULTICAST_JOIN){
			printf("[Server]: %s \n",buff);
			strcpy(my_salon, msgstruct.infos);
			return 1;
		}	

		//MULTICAST_QUIT
		if (msgstruct.type == MULTICAST_QUIT){
			printf("[Server]: %s \n",buff);
			strcpy(my_salon, msgstruct.infos);
			return 1;
		}

		//MULTICAST_LIST
		if (msgstruct.type ==MULTICAST_LIST){
			printf("[Server]: Channels are : \n%s\n",buff);
			return 1;
		}

		//MULTICAST_MEMBERS
		if (msgstruct.type ==MULTICAST_MEMBERS){
			if (strlen(buff)==0){
				printf("[Server]: Channel doesn't exist\n");
				return 1;
			}
			
			printf("[Server]: Members of channel %s are : \n%s\n",msgstruct.infos,buff);
			return 1;
		}

		//MULTICAST_SEND
		if (msgstruct.type ==MULTICAST_SEND){
			printf("[%s] : %s\n", msgstruct.nick_sender,buff);
			return 1;
		}

		//FILE_REQUEST
		if (msgstruct.type == FILE_REQUEST){
			printf("[Server] : %s wants you to accept the transfer of the file named \"%s\". \nDo you accept? [Y/N]\n",msgstruct.nick_sender,buff);
			if (send_answer(sockfd_server,fds,my_nickname,msgstruct.nick_sender)==0)
				return 0;
			return 1;
		}

		//FILE_REJECT
		if (msgstruct.type == FILE_REJECT){
			printf("[Server] : %s cancelled file transfer.\n",msgstruct.nick_sender);
			memset(filename,0, strlen(filename));
			return 1;		
		}

		//FILE_ACCEPT
		if (msgstruct.type == FILE_ACCEPT){
			printf("[Server] : %s accepted file transfert.\n",msgstruct.nick_sender);
			return file_accept(msgstruct.nick_sender,buff,my_nickname,filename);
		}

		//FILE_ACK
		if (msgstruct.type == FILE_ACK){
			memset(filename,0,strlen(filename));
			printf("[INFOS] : %s has received the file.\n",msgstruct.nick_sender);
			return 1;		
		}
	}	
	return 0;
}

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

int main(int argc, char *argv[]) {
	if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_name> <server_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

	char * server_name=argv[1];
	char * server_port=argv[2];

	int sfd;
	sfd = handle_connect(server_port,server_name);
	printf("Connecting to server ... done!\n");
	
    // Initializing the array of pollfd structures
    struct pollfd fds[CONN_CLI];
    memset(fds, 0, CONN_CLI * sizeof(struct pollfd));

    // Inserting sfd in the table
    fds[0].fd = sfd;
    fds[0].events = POLLIN;

	// Insertion of the standars entry in the table
    fds[1].fd = fileno(stdin);
    fds[1].events = POLLIN;

	// Initialization of the client parameters
	int ret;
	char my_salon[NICK_LEN] = "";
	char my_nickname[NICK_LEN]="";
	char filename[NICK_LEN]="";

	help();

	while (1) {
        // Call for poll in anticipation of new events
        int active_fds = poll(fds, CONN_CLI, -1);
		if (active_fds<0){
			perror("poll");
            exit(EXIT_FAILURE);
		}

		// Server-related socket activity
		if ((fds[0].revents & POLLIN) == POLLIN) {
			ret=echo_client(fds,fds[0].fd,fds[0].fd,fds[1].fd,my_nickname, my_salon,filename);
			if (ret==0){
				perror("echo_client");
				exit(EXIT_FAILURE);
			}

			printf("\n");
			if (strlen(my_salon)>0){
				printf("[%s]",my_salon);
			}

			if (strlen(my_nickname)>0){
				printf("[%s]",my_nickname);
			}
			
			printf(">");
			fflush(stdout);
		}

		// Activity on standard input socket
		if ((fds[1].revents & POLLIN) == POLLIN) {
			ret=echo_client(fds,fds[1].fd,fds[0].fd,fds[1].fd,my_nickname, my_salon,filename);
			if (ret==0){
				perror("echo_client");
				exit(EXIT_FAILURE);
			}
			printf("\n");
			if (strlen(my_salon)>0){
				printf("[%s]",my_salon);
			}

			if (strlen(my_nickname)>0){
				printf("[%s]",my_nickname);
			}
			
			printf(">");
			fflush(stdout);
		}
	 }
	close(sfd);
	return EXIT_SUCCESS;
}

