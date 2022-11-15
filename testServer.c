#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "inet.h"
#define DEBUG 1
int main(int argc, char **argv)
{
    struct sockaddr_in cli_addr, serv_addr, dserv_addr; /* connection
structs */
    socklen_t clilen; /* length of cli_addr struct */
    int sockfd, newsockfd, clisockfd; /* sockets */
    fd_set readset, writeset, errorset; /* socket sets for select */
    char request[MAX]; /* incoming buffer -- you will need more than one */
    char response[MAX]; /* outgoing buffer -- you will need more than one */
    int n, nread, nwritten; /* byte counters */
    char    *toiptr, *tooptr, *friptr, *froptr; /* positional pointers */
/* Create communication endpoint */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server: can't open stream socket");
        return(1);
    }
/* Bind socket to local address */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family   = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port   = htons(0); /* You will need to get this value from
the command line */
/* Set up the address of the directory server */
    memset((char *) &dserv_addr, 0, sizeof(dserv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR); /* hard-coded in
inet.h */
    serv_addr.sin_port        = htons(SERV_TCP_PORT); /* hard-coded in inet.h */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("server: can't bind local address");
        return(1);
    }
/* TODO: CONNECT TO, AND REGISTER WITH, THE DIRECTORY SERVER */
/* now we're ready to start accepting client connections */
    listen(sockfd, 5);
/* initialize buffer pointers */
    toiptr = tooptr = response;
    friptr = froptr = request;
    for ( ; ; ) {
        FD_ZERO(&readset);
        FD_SET(sockfd, &readset);
        FD_SET(sockfd, &errorset);
        int max_fd = sockfd;
/* TODO: POPULATE READ, WRITE, AND ERROR SETS WITH CLIENT SOCKETS HERE 
*/
        if ((n=select(max_fd+1, &readset, &writeset, &errorset, NULL)) > 0) {
            if (FD_ISSET(sockfd, &readset)) {
/* Accept a new connection request. */
                clilen = sizeof(cli_addr);
                newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
                                   &clilen);
                if (newsockfd < 0) {
                    perror("server: accept error");
                    exit(1);
                }
                if (fcntl(newsockfd, F_SETFL, O_NONBLOCK) != 0 ) {
                    perror("server: couldn't set new client socket to nonblocking");
                    exit(1);
                }
/* Modified example code from UNIX Socket Programming 
Example 16.4 */
/* Read the request from the client. */
                if ((nread = read(newsockfd, friptr, &request[MAX] - friptr)) < 0) {
                    if (errno != EWOULDBLOCK) {
                        perror("server: read error on new socket");
                        exit(1);
                    } else if (nread == 0) {
                        fprintf(stderr, "EOF on new socket; maybe the client closed the connection?\n");
                        close(newsockfd);
                        continue; /* Keep going through the fot loop from the beginning */
                    } else {
                        fprintf(stderr, "server: read %d bytes from socket\n", nread);
                        friptr += nread; /* bytes just read */
                    }
                }
/* Compose response */
            }
/* TODO: READ FROM CLIENT SOCKETS HERE */
/* clisockfd is used as an example socket */
/* if (FD_ISSET(clisockfd, &readset)) { */
/* TODO: WRITE TO CLIENT SOCKETS HERE */
/* Modified example code from UNIX Socket Programming Example 
16.4 */
/* clisockfd is used as an example socket */
            if (FD_ISSET(clisockfd, &writeset) && ((n = toiptr - tooptr) >
                                                   0)) {
                if ((nwritten = write(sockfd, tooptr, n)) < 0) {
                    if (errno != EWOULDBLOCK)
                        perror("server: write error on client socket");
                    exit(1);
                } else {
                    fprintf(stderr, "server: wrote %d bytes to socket\n",
                            nwritten);
                    tooptr += nwritten; /* bytes just written */
                    if (tooptr == toiptr) {
                        toiptr = tooptr = response;   /* back to
beginning of buffer */
                    }
                }
            }
        } /* end of if select */
    } /* end of infinite for loop */
#if DEBUG
    fprintf(stderr, "\nServer is exiting...\n");
#endif
    close(sockfd);
//return or exit(0) is implied; no need to do anything because main() ends
}