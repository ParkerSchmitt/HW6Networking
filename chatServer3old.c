/*-------------------------------------------------------------*/
/* server.c - Multi-threaded, concurrent time/date server.     */
/*-------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "inet.h"

#define DEBUG 1

void *handle_request(void *); /* new thread's code */
int connect_to_directory_server(char*, int, int*);
int send_info_to_directory_server(int, char*, char*);
char* serverName = NULL;
char* serverPort = NULL;

static char names[MAX_CLIENTS][255];
static int namesIndex[MAX_CLIENTS];

int sockfd;
int dir_sock;
pthread_t *threads;
unsigned int *threads_ids;

pthread_mutex_t namesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t namesIndexMutex = PTHREAD_MUTEX_INITIALIZER;




int main(int argc, char **argv)
{

    int                 sockfd, newsockfd, childpid;
    unsigned int	clilen;
    struct sockaddr_in  cli_addr, serv_addr;
    struct tm* timeptr;  /* pointer to time structure */
    time_t              clock;     /* clock value (in secs)     */
    char                s[MAX];
    char                request;


    if (argc != 3) {
        printf("chatServer2 must be ran as ./chatServer2 [NAME] [PORT]. Please provide all arguments.\n");
        exit(1);
    }
    else {
        serverName = argv[1];
        serverPort = argv[2];
        printf("%s\n", serverName);
    }

    /* Create communication endpoint */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server: can't open stream socket");
        exit(1);
    }

    /* Bind socket to local address */
    memset((char*)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
    serv_addr.sin_port = htons(atoi(serverPort));

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("server: can't bind local address");
        exit(1);
    }

    //Let the directory server know that we are online!
    int dir_sock;
    if (connect_to_directory_server(DIR_SERVER_ADDR, DIR_SERVER_ADDR_PORT, &dir_sock) == 0) {
        printf("\nCan't connect to directory server.");
        exit(-1);
    }
    //Send the directory server our name and address if connection was succesful
    int response;
    response = send_info_to_directory_server(dir_sock, serverName, serverPort);
    if (response != 0) {
        printf("\nThat server name exists, please try again.\n");
        exit(-1);
    }

    listen(sockfd, MAX_CLIENTS);

    char names[MAX_CLIENTS][255];
    int namesIndex[MAX_CLIENTS];
    int namesBuffer[MAX_CLIENTS][MAX];
    int max_client_FD = 0;
    int nameCount = 0;
    fd_set current_sockets;
    FD_ZERO(&current_sockets);
    FD_SET(sockfd,&current_sockets);

    for (; ;) {
        fd_set copy = current_sockets;

        if(select(FD_SETSIZE, &copy, NULL, NULL, NULL) < 0)  {
            printf("select error");
            exit(0);
        }

        for (int i=0; i < FD_SETSIZE; i++) {
                if (FD_ISSET(i, &copy)) {

                if (i == sockfd) {

                    //New connection
                    int clientSock = accept(sockfd, NULL, NULL );
                    if (fcntl(newsockfd, F_SETFL, O_NONBLOCK) != 0 ) {
                        perror("server: couldn't set new client socket to nonblocking");
                        exit(1);
                    }
                    
                    max_client_FD = max(max_client_FD,clientSock);
                    FD_SET(clientSock, &current_sockets);
                } else {
                    //Message   
                    char buff[255] = {0};
                    strcpy(buff, "");
                    int bytes = recv(i,buff, 255,0);
                   printf("%s\n",buff);

                    if (bytes <= 0) {
                        //Drop
                        close(i);
                        FD_CLR(i, &current_sockets);
                    } else {

                        if (buff[0]=='n') { //A new user joined and gave us their name.Lets output it to other users

                            //See if name is a duplicate
                            char* name = buff + 1; 
                            int dupe = 0; // 0 if false 1 if true
                            for (int j=0; j < 5; j++) {
                                if (name == names[j]) {
                                    dupe = 1;
                                }
                            }
                            if (dupe == 0 && nameCount != 4) {
                                write(i,"0", 1); // 1 is good
                                strcpy(names[nameCount], name);
                                namesIndex[nameCount] = i;
                                nameCount++;
                                printf("added\n");
                            } else {
                                write(i,"1", 1); // 1 is good
                                //Also add it to the list
                            }



                            for (int j = 0; j < 1024; j++) {
                                if (FD_ISSET(j,&current_sockets)) {
                                    char newUserMessage[255];
                                    //printf("%s\n",buff);
                                    snprintf(newUserMessage, 255, "%s has joined the server\n", name );
                                    if (j != sockfd && j != i) {
                                        send(j, newUserMessage, strlen(newUserMessage),0);
                                    }
                                }
                            }
                        }


                        if (buff[0]=='m') {
                            char* message = buff + 1;
                            char* name;
                            int userIndex;
                            //Find the name for this connection
                            for (int j=0; j < 5; j++) {
                                if (namesIndex[j] == i) {
                                    name = names[j];
                                    userIndex = j;
                                }
                            }
                                char newUserMessage[255];
                                snprintf(newUserMessage, 255, "%s: %s\n", name, message);
                            int done =0;
                            while (!done) {
                                for (int j = 0; j < MAX_CLIENTS; j++) {
                                    done = 1; //assume this finshes up the writing
                                    if (FD_ISSET(namesIndex[j],&current_sockets)) {
                                        if (j != i) {
                                        
                                            if (pos[i] < size)
                                            bytes = send(clientIndex[j], newUserMessage+pos[j],size-pos[j],0);
                                            if (bytes > 0) {
                                                pos[j] += bytes;
                                                if (pos[i] < size) {
                                                    done = 0;
                                                }
                                            }
                                        }
                                    }
                                }


                            }
                        }
                    }
                }
            }
        }
    }

}


int send_info_to_directory_server(int sockfd, char* name, char* port) {
    char sendMsg[strlen(name) + strlen(port) + 2];
    snprintf(sendMsg, 30, "n%s %s", name, port);

    write(sockfd, &sendMsg, sizeof(char) * (strlen(name) + strlen(port) + 2)); //Add two because the first char is going to be a letter plus a space
    //Read back the response. 0 if name is good and 1 if its bad (someone else has it)
    /* Read the server's reply. */
    int nread;
    char test[255];

    nread = recv(sockfd, test, MAX, 0);
    if (nread > 0) {
        if (test[0] == '0') {
            return 0;
        }
        else {
            return 1;
        }
    }
    else {
        //printf("\nNothing read.");
        return 0;
    }
}


int connect_to_directory_server(char* server_address, int port, int* returnsocket) {

    int                 sockfd;
    struct sockaddr_in  serv_addr;
    char                s[MAX];      /* array to hold output */
    int                 response;    /* user response        */
    int                 nread;       /* number of characters */

    /* Set up the address of the server to be contacted. */
    memset((char*)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(server_address);
    serv_addr.sin_port = htons(port);
    /* Display the menu, read user's response, and send it to the server. */

    /* Create a socket (an endpoint for communication). */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("client: can't open stream socket");
        return 1;
    }
    //fcntl(sockfd, F_SETFL, O_NONBLOCK);

    fd_set fdset;
    struct timeval tv;
    FD_ZERO(&fdset);
    FD_SET(sockfd, &fdset);
    tv.tv_sec = 5;             /* 5 second timeout */
    tv.tv_usec = 0;
    connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    if (select(sockfd + 1, NULL, &fdset, NULL, &tv) == 1)
    {
        int so_error;
        socklen_t len = sizeof so_error;
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
        *returnsocket = sockfd;

        if (so_error != 0) {
            return 0;
        }
        return 1;
    }
}
