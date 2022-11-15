/*------------------------------------------------------*/
/* Ex32.c - select on stdin                             */
/*------------------------------------------------------*/
#include <stdio.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "inet.h"
#include <string.h>
#include <stdlib.h>




void sighandler(int);    /* Signal handling function prototype */
int get_response(void);
int connect_to_server(char*, int, int*);
int send_name_to_server(int, char*);

int get_server_names(int, char*);

int main()
{
    printf("Avaliable servers:\n");
    printf("TOPIC   |   ADDRESS\n");
    printf("-------------------\n");
    int sockfd;

    if (connect_to_server(DIR_SERVER_ADDR,DIR_SERVER_ADDR_PORT, &sockfd) == 0) {
        printf("Can't connect to directory server.\n");
        exit(-1);
    }

    write(sockfd, "g", 1);

    for (; ; ) {
        int nread;
        char test[255];

        nread = recv(sockfd, test, MAX, 0);
        if (nread > 0) {
            printf("%s\n", test);
        }
        else {
            //printf("\nNothing read.");
            break;
        }
    }

    char serverStr[255];
    printf("Enter a server to connect to:");
    //scanf("%s\n", serverStr);  
    fgets(serverStr, 255, stdin);
    char serverAddr[245];
    int serverPort;
    sscanf(serverStr, "%[^:]:%d", serverAddr, &serverPort);

    printf("\nConnecting...");
    if (connect_to_server(serverAddr, serverPort, &sockfd) == 0) {
        printf("\nCan't connect to server.");
        exit(-1);
    }


    for (; ;) {

        printf("\nPlease enter a name for the server. MAX 20 chars:");
        char nameStr[20];
        //scanf("%s\n", nameStr);
        fgets(nameStr, 20, stdin);
        nameStr[strcspn(nameStr, "\n")] = 0;
        int response;
        printf("\nJoining...\n");

        response = send_name_to_server(sockfd, nameStr);

        if (response == 0) {

            break;
        }
        else {
            printf("\nThat name already exists, please try again.\n");
        }

    }

    fd_set ready;
    int ret;
    char responseStr[255] = { 0 };
    char* inputStr = malloc(255);
    int i;
    int len;
    for (; ; ) {
        FD_ZERO(&ready);
        FD_SET(0, &ready);
        FD_SET(sockfd, &ready);
        strcpy(responseStr, "");

        if ((i = select(sockfd + 1, &ready, NULL, NULL, NULL)) > 0) {
            if (FD_ISSET(0, &ready)) {
                //User input
                fgets(inputStr, 255, stdin);
                /* Remove trailing newline character from the input buffer if needed. */
                len = strlen(inputStr) - 1;
                if (inputStr[len] == '\n') {
                    inputStr[len] = '\0';
                }
                char sendMsg[256];
                snprintf(sendMsg, 256, "m%s", inputStr);
                write(sockfd, &sendMsg, sizeof(char) * (strlen(sendMsg) + 1));
            }

            if (FD_ISSET(sockfd, &ready)) {
                //Server input
                if ((ret = recv(sockfd, responseStr, MAX, 0)) > 0) {
                    responseStr[ret] = '\0';
                    printf("%s", responseStr);
                }
            }

        }
    }

    exit(1);
    close(sockfd);

}

int send_name_to_server(int sockfd, char* name) {
    char sendMsg[strlen(name) + 1];
    snprintf(sendMsg, 21, "n%s", name);

    write(sockfd, &sendMsg, sizeof(char) * (strlen(name) + 1)); //Add one because the first char is going to be a letter
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
        printf("\nNothing read.");
        return 0;
    }
}



int get_server_names(int sockfd, char* name) {
    char sendMsg[strlen(name) + 1];
    //snprintf(sendMsg, 21, "g");

    write(sockfd, "g", sizeof(char) * (strlen(name) + 1)); //Add one because the first char is going to be a letter
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
        printf("\nNothing read.");
        return 0;
    }
}


int connect_to_server(char* server_address,int port, int* returnsocket) {

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
    /* Connect to the server. */
    if (connect(sockfd, (struct sockaddr*)&serv_addr,
        sizeof(serv_addr)) < 0) {
        perror("client: can't connect to server");
        exit(1);
    }


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
