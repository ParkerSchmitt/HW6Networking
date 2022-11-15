/*-------------------------------------------------------------*/
/*chatServer2.c - chat server that connects to directory server*/
/*-------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include "inet.h"
/* According to POSIX.1-2001 */
#include <sys/select.h>


#include <unistd.h>
int connect_to_directory_server(char*, int*);
int send_name_to_directory_server(int, char*, char*);
const char* serverName = NULL;
const char* serverPort = NULL;


int main(int argc, char** argv)
{
    int                 sockfd, newsockfd, childpid;
    unsigned int	clilen;
    struct sockaddr_in  cli_addr, serv_addr;
    struct tm* timeptr;  /* pointer to time structure */
    time_t              clock;     /* clock value (in secs)     */
    char                s[MAX];
    char                request;

    /* Create communication endpoint */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server: can't open stream socket");
        exit(1);
    }
    /* Bind socket to local address */
    memset((char*)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(DIR_SERVER_ADDR_PORT);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("server: can't bind local address");
        exit(1);
    }

    listen(sockfd, MAX_CLIENTS);
    char names[MAX_CLIENTS][255];
    int namesIndex[MAX_CLIENTS];
    memset(names[0], '\0', strlen(names[0]));
    memset(names[1], '\0', strlen(names[1]));
    memset(names[2], '\0', strlen(names[2]));
    memset(names[3], '\0', strlen(names[3]));
    memset(names[4], '\0', strlen(names[4]));
    memset(names[5], '\0', strlen(names[5]));
    int nameCount = 0;
    fd_set current_sockets;
    FD_ZERO(&current_sockets);
    FD_SET(sockfd, &current_sockets);

    for (; ;) {
        fd_set copy = current_sockets;

        if (select(FD_SETSIZE, &copy, NULL, NULL, NULL) < 0) {
            printf("select error");
            exit(0);
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            if (FD_ISSET(i, &copy)) {

                if (i == sockfd) {
                    clilen = sizeof(cli_addr);

                    //New connection
                    int clientSock = accept(sockfd, (struct sockaddr*) &cli_addr, &clilen);
                    FD_SET(clientSock, &current_sockets);
                }
                else {
                    //Message   
                    char buff[255] = { 0 };

                    strcpy(buff, "");
                    int bytes = recv(i, buff, 255, 0);
                    //printf("%s\n", buff);

                    if (bytes <= 0) {
                        //Drop

                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (namesIndex[j] == i) {
                                // add mutex lock names
                                strcpy(names[j], "");
                                namesIndex[j] = 0;
                                close(i);
                                FD_CLR(i, &current_sockets);

                            }
                        }
                    }
                    else {

                        if (buff[0] == 'n') { //A new server joined

                            //See if name is a duplicate
                            char* name = buff + 1;
                            int dupe = 0; // 0 if false 1 if true
                            for (int j = 0; j < 5; j++) {
                                if (strcmp(name,names[j]) == 0 ) {
                                    dupe = 1;
                                }
                            }

                            for (int j = 0; j < 5; j++) {

                                if (dupe == 0 && nameCount != 4) {
                                    char serverInfo[255];
                                    char portString[25];
                                    char topicString[128];
                                    sscanf(name, "%s %s", topicString, portString);
                                    if (strlen(names[j]) == 0) {

                                        snprintf(serverInfo, 255, "%s | %s:%s", topicString, inet_ntoa(cli_addr.sin_addr), portString);
                                        strcpy(names[j], serverInfo);
                                        //printf("%s",serverInfo);
                                        write(i, "0", 1); // 1 is good
                                        break;
                                    }
                                }
                            }
							write(i, "1", 1); // 1 is good

                        }



                        // 'g' = get servers
                        if (buff[0] == 'g') {
                            //Find the name for this connection
                            for (int j = 0; j < 5; j++) {
                                if (strlen(names[j]) != 0) {
                                    char stringFormat[256];
                                    memset(stringFormat, '\0', strlen(stringFormat));
                                    snprintf(stringFormat, 256, "%s\n", names[j]);

                                    send(i, stringFormat, strlen(stringFormat), 0);
                                }
                            }
                            //We don't need this connection open anymore- client has all info now 
                            close(i);
                            FD_CLR(i, &current_sockets);
                        }
                    }
                }
            }
        }
    }
}
