/******************************************************************************
* echo_server.c                                                               *
*                                                                             *
* Description: This file contains the C source code for an echo server.  The  *
*              server runs on a hard-coded port and simply write back anything*
*              sent to it by connected clients.  It does not support          *
*              concurrent clients.                                            *
*                                                                             *
* Authors: Athula Balachandran <abalacha@cs.cmu.edu>,                         *
*          Wolf Richter <wolf@cs.cmu.edu>                                     *
*                                                                             *
*******************************************************************************/

#include <netinet/in.h>
#include "utility.h"
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>

int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

/* Start listening socket listen_sock. */
int start_listen_socket(int backlog_num, int reuse)
{
    int listen_sock; // a file descriptor for our "listening" socket.
    if ((listen_sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
        perror("Failed setting socket options");
        return -1;
    }
    // create a struct storing address information for the socket
    struct sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(ECHO_PORT); // use default echo server port
    sock_addr.sin_addr.s_addr = INADDR_ANY;

    // bind socket with the address
    if (bind(listen_sock, (struct sockaddr *) &sock_addr, sizeof(sock_addr)))
    {
        close_socket(listen_sock);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }
    // listen(int socket, int backlog);
    // The backlog parameter defines the maximum length for the queue
    // of pending connections.
    if (listen(listen_sock, backlog_num))
    {
        close_socket(listen_sock);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }
    printf("Accepting connections on port %d.\n", (int)ECHO_PORT);
    return 0;
}

int main(int argc, char* argv[])
{
    int client_sock;
    ssize_t readret;
    socklen_t cli_size;
    struct sockaddr_in cli_addr;
    memset(&cli_addr, 0, sizeof(cli_addr));
    char buf[BUF_SIZE];

    fprintf(stdout, "----- Echo Server -----\n");
    start_listen_socket(BACKLOG, SOCK_REUSE);
    /* finally, loop waiting for input and then write it back */
    while (1)
    {
       cli_size = sizeof(cli_addr);
       if ((client_sock = accept(sock, (struct sockaddr *) &cli_addr,
                                 &cli_size)) == -1)
       {
           close(sock);
           fprintf(stderr, "Error accepting connection.\n");
           return EXIT_FAILURE;
       }
       
       readret = 0;

       while((readret = recv(client_sock, buf, BUF_SIZE, 0)) >= 1)
       {
           if (send(client_sock, buf, readret, 0) != readret)
           {
               close_socket(client_sock);
               close_socket(sock);
               fprintf(stderr, "Error sending to client.\n");
               return EXIT_FAILURE;
           }
           memset(buf, 0, BUF_SIZE);
       } 

       if (readret == -1)
       {
           close_socket(client_sock);
           close_socket(sock);
           fprintf(stderr, "Error reading from client socket.\n");
           return EXIT_FAILURE;
       }

       if (close_socket(client_sock))
       {
           close_socket(sock);
           fprintf(stderr, "Error closing client socket.\n");
           return EXIT_FAILURE;
       }
    }

    close_socket(sock);

    return EXIT_SUCCESS;
}
