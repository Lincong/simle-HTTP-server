//
// Created by Lincong Li on 1/4/18.
//

#include "utility.h"

/* Start listening socket listen_sock. */
int start_listen_socket(int backlog_num, int reuse, int* sock_fd)
{
    if((*sock_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }
    if(setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0) {
        fprintf(stderr, "Failed setting socket options\n");
        return -1;
    }
    // create a struct storing address information for the socket
    struct sockaddr_in sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(ECHO_PORT); // use default echo server port
    sock_addr.sin_addr.s_addr = INADDR_ANY;

    // bind socket with the address
    if(bind(*sock_fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)))
    {
        close_socket(*sock_fd);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }
    // listen(int socket, int backlog);
    // The backlog parameter defines the maximum length for the queue
    // of pending connections.
    if(listen(*sock_fd, backlog_num))
    {
        close_socket(*sock_fd);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }
    printf("Accepting connections on port %d.\n", (int)ECHO_PORT);
    return 0;
}

int close_socket(int sock_fd)
{
    if (close(sock_fd))
    {
        perror("Failed closing socket.\n");
        return 1;
    }
    return 0;
}
