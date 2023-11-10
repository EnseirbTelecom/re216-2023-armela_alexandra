#define MSG_LEN 1024
#define SERV_PORT "8080"
#define SERV_ADDR "127.0.0.1"

#define MAX_CONN 128
#define MAX_CHANNEL 10 

#define CONN_CLI 2

struct Client {
    int sockfd;
	char* nickname;
	char* salon;
	int i_salon;
    struct in_addr addr;
	unsigned short int port;
	time_t connectionTime;
    struct Client * next;
};

struct Channel {
	int nbr_client;
	char* name ;
};

