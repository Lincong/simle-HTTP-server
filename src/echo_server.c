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

#include "utility.h"
#include "message.h"

int echo_sock_fd; // a file descriptor for our "listening" socket.
peer_t connection_list[MAX_CLIENTS];

void server_shutdown_properly(int code);
void handle_signal_action(int sig_number);
int setup_signals();
int build_fd_sets(int listen_sock, fd_set *read_fds, fd_set *write_fds, fd_set *except_fds);

int main(int argc, char* argv[])
{
    if (setup_signals() != 0)
        exit(EXIT_FAILURE);

    if (start_listen_socket(BACKLOG, SOCK_REUSE, &echo_sock_fd) != 0)
        exit(EXIT_FAILURE);

    /* Set nonblock for stdin. */
//    int flag = fcntl(STDIN_FILENO, F_GETFL, 0);
//    flag |= O_NONBLOCK;
//    fcntl(STDIN_FILENO, F_SETFL, flag);

    int i;
    for (i = 0; i < MAX_CLIENTS; ++i) {
        connection_list[i].socket = NO_SOCKET;
        create_peer(&connection_list[i]);
    }

    fd_set read_fds;
    fd_set write_fds;
    fd_set except_fds;

    int high_sock = echo_sock_fd;
    LOG("Waiting for incoming connections.");

    // loop waiting for input and then write it back
    while (1)
    {
        LOG("Building fd sets...");
        build_fd_sets(echo_sock_fd, &read_fds, &write_fds, &except_fds);
        LOG("Done");

        LOG("Get highest socket number...");
        high_sock = echo_sock_fd;
        for (i = 0; i < MAX_CLIENTS; ++i) {
            if (connection_list[i].socket > high_sock)
                high_sock = connection_list[i].socket;
        }
        LOG("Done");

        LOG("Selecting...");
        int activity = select(high_sock + 1, &read_fds, &write_fds, &except_fds, NULL);
        LOG("Done");

        switch (activity) {
            case -1:
                perror("select()");
                server_shutdown_properly(EXIT_FAILURE);

            case 0:
                // you should never get here
                printf("select() returns 0.\n");
                server_shutdown_properly(EXIT_FAILURE);

            default:

                if (FD_ISSET(echo_sock_fd, &read_fds)) {
                    handle_new_connection(echo_sock_fd, connection_list);
                }

                if (FD_ISSET(STDIN_FILENO, &except_fds)) {
                    printf("except_fds for stdin.\n");
                    server_shutdown_properly(EXIT_FAILURE);
                }

                if (FD_ISSET(echo_sock_fd, &except_fds)) {
                    printf("Exception listen socket fd.\n");
                    server_shutdown_properly(EXIT_FAILURE);
                }

                for (i = 0; i < MAX_CLIENTS; ++i) {
                    if (connection_list[i].socket != NO_SOCKET && FD_ISSET(connection_list[i].socket, &read_fds)) {
                        if (receive_from_peer(&connection_list[i], &echo_received_message) != EXIT_SUCCESS) {
                            close_client_connection(&connection_list[i]);
                            continue;
                        }
                    }

                    if (connection_list[i].socket != NO_SOCKET && FD_ISSET(connection_list[i].socket, &write_fds)) {
                        if (send_to_peer(&connection_list[i]) != EXIT_SUCCESS) {
                            close_client_connection(&connection_list[i]);
                            continue;
                        }
                    }

                    if (connection_list[i].socket != NO_SOCKET && FD_ISSET(connection_list[i].socket, &except_fds)) {
                        printf("Exception client fd.\n");
                        close_client_connection(&connection_list[i]);
                        continue;
                    }
                }
        }

        printf("And we are still waiting for clients' or stdin activity. You can type something to send:\n");
    }

    return EXIT_SUCCESS;
}

void server_shutdown_properly(int code)
{
    int i;

    close(echo_sock_fd);

    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (connection_list[i].socket != NO_SOCKET)
            delete_peer(&connection_list[i]); // de-allocate memory for each peer
    }
    printf("Shutdown server properly.\n");
    exit(code);
}


void handle_signal_action(int sig_number)
{
    if (sig_number == SIGINT) {
        printf("SIGINT was catched!\n");
        server_shutdown_properly(EXIT_SUCCESS);
    }
    else if (sig_number == SIGPIPE) {
        printf("SIGPIPE was catched!\n");
        server_shutdown_properly(EXIT_SUCCESS);
    }
}

int setup_signals()
{
    struct sigaction sa;
    sa.sa_handler = handle_signal_action;
    if (sigaction(SIGINT, &sa, 0) != 0) {
        perror("sigaction()");
        return -1;
    }
    if (sigaction(SIGPIPE, &sa, 0) != 0) {
        perror("sigaction()");
        return -1;
    }

    return 0;
}

int build_fd_sets(int listen_sock, fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
{
    int i;

    FD_ZERO(read_fds);
    FD_SET(STDIN_FILENO, read_fds);
    FD_SET(listen_sock, read_fds);
    for (i = 0; i < MAX_CLIENTS; ++i)
        if (connection_list[i].socket != NO_SOCKET)
            FD_SET(connection_list[i].socket, read_fds);

    FD_ZERO(write_fds);
    for (i = 0; i < MAX_CLIENTS; ++i)
        if (connection_list[i].socket != NO_SOCKET && connection_list[i].is_sending)
            FD_SET(connection_list[i].socket, write_fds);

    FD_ZERO(except_fds);
    FD_SET(STDIN_FILENO, except_fds);
    FD_SET(listen_sock, except_fds);
    for (i = 0; i < MAX_CLIENTS; ++i)
        if (connection_list[i].socket != NO_SOCKET)
            FD_SET(connection_list[i].socket, except_fds);

    return 0;
}