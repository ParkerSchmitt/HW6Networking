#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "inet.h"
#define DEBUG 1


void *handle_request(void *); /* new thread's code */
int connect_to_directory_server(char*, int, int*);
int send_info_to_directory_server(int, char*, char*);

int main(int argc, char **argv)
{
    struct sockaddr_in cli_addr, serv_addr, dserv_addr; /* connection
structs */
    socklen_t clilen; /* length of cli_addr struct */
    int sockfd, newsockfd, clisockfd; /* sockets */
    fd_set readset, writeset, errorset; /* socket sets for select */
    char request[MAX_CLIENTS][MAX]; /* incoming buffer -- you will need more than one */
    char response[MAX_CLIENTS][MAX]; /* outgoing buffer -- you will need more than one */
    int n, nread, nwritten; /* byte counters */
    char    *toiptr[MAX_CLIENTS], *tooptr[MAX_CLIENTS], *friptr[MAX_CLIENTS], *froptr[MAX_CLIENTS]; /* positional pointers */
    char names[MAX_CLIENTS][255];
    int namesIndex[MAX_CLIENTS];
    int nameCount = 0;

	
	char* serverName = NULL;
	char* serverPort = NULL;
	
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
        return(1);
    }
/* Bind socket to local address */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family   = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port   = htons(SERV_TCP_PORT); /* You will need to get this value from the command line */
/* Set up the address of the directory server */
    memset((char *) &dserv_addr, 0, sizeof(dserv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR); /* hard-coded in
inet.h */
    serv_addr.sin_port        = htons(atoi(serverPort)); /* hard-coded in inet.h */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("server: can't bind local address");
        return(1);
    }


	//Let the directory server know that we are online!
    int dir_sock;
    if (connect_to_directory_server(DIR_SERVER_ADDR, DIR_SERVER_ADDR_PORT, &dir_sock) == 0) {
        printf("\nCan't connect to directory server.");
        exit(-1);
    }

    if (send_info_to_directory_server(dir_sock, serverName, serverPort) != 0) {
        printf("\nThat server name exists, please try again.\n");
        exit(-1);
    }
	

/* now we're ready to start accepting client connections */
    listen(sockfd, MAX_CLIENTS);
/* initialize buffer pointers */

for (int i=0; i < MAX_CLIENTS; i++) {
    toiptr[i] = tooptr[i] = response[i];
    friptr[i] = froptr[i] = request[i];
    namesIndex[i] = -1;
    memset(request[i],0,sizeof(request[i]));

}

    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_ZERO(&errorset);

    FD_SET(sockfd,&readset);


    for (; ;) {
        fd_set readcopy = readset;
        fd_set writecopy = writeset;
        fd_set errorcopy = errorset;


        if(select(FD_SETSIZE, &readcopy, &writecopy, &errorcopy, NULL) < 0)  {
            printf("select error");
            exit(0);
        }

        for (int i=0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &readcopy)) {

                if (i == sockfd) {

                    //New connection
                    int clientSock = accept(sockfd, NULL, NULL);
                    if (fcntl(clientSock, F_SETFL, O_NONBLOCK) != 0) {
                        perror("server: couldn't set new client socket to nonblocking");
                        exit(1);
                    }

                    FD_SET(clientSock, &readset);

                    //Find a empty place to put our new index
                    for (int j=0; j < MAX_CLIENTS; j++) {
                        if (namesIndex[j] == -1) {
                            namesIndex[j] = clientSock;
                            nameCount++;
                            break;
                        }
                    }

                } else {

                    int connIndex = 0;
                    for (int j=0; j < MAX_CLIENTS; j++) {
                        if (namesIndex[j] == i) {
                            connIndex = j;
                        }
                    }

                    if ((nread = read(i, friptr[connIndex], &request[connIndex][MAX] - friptr[connIndex])) < 0) {
                            if (errno != EWOULDBLOCK) {
								FD_SET(i,&errorset);
                                perror("server: read error on new socket\n");
                                exit(1);
                            }
                    } else if (nread == 0) {
                                fprintf(stderr, "EOF on new socket; maybe the client closed the connection?\n");
                                close(i);
								memset(names[i],0,sizeof(names[i]));
								memset(request[i],0,sizeof(request[i]));
								namesIndex[i] = -1;
								nameCount--;
								FD_SET(i,&errorset);
                                FD_CLR(i, &readset);

                                continue; /* Keep going through the fot loop from the beginning */
                    } else {
                                //fprintf(stderr, "server: read %d bytes from socket\n", nread);
                                friptr[connIndex] += nread; /* bytes just read */

                                //is the message over?
                                //fprintf(stderr, "last char: %c\n", *(friptr[connIndex]));
                                if (*(friptr[connIndex]) == '\0')
                                {
                                    //fprintf(stderr, "found: %s\n", request[connIndex]);
                                    if (request[connIndex][0]=='n') { //A new user joined and gave us their name

                                        //See if name is a duplicate
                                        char* name = request[connIndex] + 1;
                                        int dupe = 0; // 0 if false 1 if true
                                        for (int j=0; j < MAX_CLIENTS; j++) {
                                            if (strcmp(name,names[j]) == 0) {
                                                dupe = 1;
                                            }
                                        }
                                        if (dupe == 0 && nameCount != MAX_CLIENTS) {
                                            FD_SET(i,&writeset);
                                            strncpy(response[connIndex], "0",1);
                                            toiptr[connIndex] += 1; // Our message is 1 byte big
                                            strcpy(names[connIndex], name);
                                            //printf("added\n");


                                            //Let the rest of the users know that the user has joined the chat server
                                            for (int j = 0; j < MAX_CLIENTS; j++) {
                                                if (namesIndex[j] != i && strcmp(names[j],"") != 0 ) {
                                                    char newUserMessage[255];
                                                    snprintf(newUserMessage, 255, "%s has joined the server\n", name);
                                                    FD_SET(namesIndex[j],&writeset);
                                                    strncpy(response[j], newUserMessage,strlen(newUserMessage));
                                                    toiptr[j] += strlen(newUserMessage);
                                                }
                                            }
                                        } else {
                                            FD_SET(i,&writeset);
                                            strncpy(response[connIndex], "1",1);
                                            toiptr[connIndex] += 1; // Our message is 1 byte big
                                        }
                                    } //end new user if

                                    //A user is trying to send a message
                                    if (request[connIndex][0] =='m') {
                                        char* message = request[connIndex] + 1;
                                        char* name = names[connIndex];
                                        char newUserMessage[255];

                                        snprintf(newUserMessage,255, "%s: %s\n", name, message);

                                        for (int j = 0; j < MAX_CLIENTS; j++) {
                                            if (namesIndex[j] != i && strcmp(names[j],"") != 0 ) {
                                                FD_SET(namesIndex[j],&writeset);
                                                strncpy(response[j], newUserMessage,strlen(newUserMessage));
                                                toiptr[j] += strlen(newUserMessage);
                                            }
                                        }
                                    }

                                    //restart over. the transmission has been received.
                                    memset(request[connIndex],0,sizeof(request[connIndex]));
                                    friptr[connIndex] = froptr[connIndex];
                                }
                    }
                }
            } //readset
			
			
            if (FD_ISSET(i, &errorset)) {
				perror("server: error caught on client.");
                FD_CLR(i, &errorset);
			} //errorset

            //If there is anything to write to
            if (FD_ISSET(i, &writeset)) {
                int connIndex = 0;
                for (int j=0; j < MAX_CLIENTS; j++) {
                    if (namesIndex[j] == i) {
                        connIndex = j;
                    }
                }
                if ((n = toiptr[connIndex] - tooptr[connIndex]) > 0) {
                    if ((nwritten = write(i, tooptr[connIndex], n)) < 0) {
                        if (errno != EWOULDBLOCK) {
                            perror("server: write error on client socket");
                            exit(1);
                      }
                   } else {
                       fprintf(stderr, "server: wrote %d bytes to socket\n", nwritten);
                       tooptr[connIndex] += nwritten; /* bytes just written */
                       if (tooptr[connIndex] == toiptr[connIndex]) { //we wrote everything
                         toiptr[connIndex] = tooptr[connIndex] = response[connIndex];   /* back to beginning of buffer */
                         FD_CLR(i, &writeset);
                       }
                   }
                }
            } //writeset
        }
         /* end of if select */
    } /* end of infinite for loop */
#if DEBUG
    fprintf(stderr, "\nServer is exiting...\n");
#endif
    close(sockfd);
//return or exit(0) is implied; no need to do anything because main() ends
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
