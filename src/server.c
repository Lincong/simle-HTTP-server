/******************************************************************************
* Authors: Lincong Li                                                         *
*******************************************************************************/

#include "utility.h"
#include "connection_handlers.h"
#include "logger.h"
#include "message.h"
#include "http.h"
#include "file_handlers.h"
#define USAGE "\nLiso Server Usage: ./lisod <HTTP port> <log file> <www folder>\n"

int http_port;
int https_port;
char *WWW_DIR;

int http_sock_fd; // a file descriptor for our "listening" socket.
peer_t connection_list[MAX_CLIENTS];

void server_shutdown_properly(int code);
void handle_signal_action(int sig_number);
int setup_signals();
int build_fd_sets(int listen_sock, fd_set *read_fds, fd_set *write_fds, fd_set *except_fds);
void init_server(int argc, char* argv[]); // set http port and other parameters

int main(int argc, char* argv[])
{

    init_server(argc, argv);

    SERVER_LOG("%s", "Setting up signal handlers...")
    if (setup_signals() != 0)
        exit(EXIT_FAILURE);
    SERVER_LOG("%s", "Done")

    if (start_listen_socket(BACKLOG, SOCK_REUSE, &http_sock_fd) != 0)
        exit(EXIT_FAILURE);

    SERVER_LOG("%s", "Initializing peers...")
    int i;
    for (i = 0; i < MAX_CLIENTS; ++i) {
        connection_list[i].socket = NO_SOCKET;
        create_peer(&connection_list[i]);
    }
    SERVER_LOG("%s", "Done")

    fd_set read_fds;
    fd_set write_fds;
    fd_set except_fds;
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 2000;

    int high_sock = http_sock_fd;

    bool has_read;
    bool has_sent;
    // loop waiting for input and then write it back
    while (1)
    {
        build_fd_sets(http_sock_fd, &read_fds, &write_fds, &except_fds);
        high_sock = http_sock_fd;
        for (i = 0; i < MAX_CLIENTS; ++i) {
            if (connection_list[i].socket > high_sock)
                high_sock = connection_list[i].socket;
        }
//        SERVER_LOG("%s", "Selecting...")
        int activity = select(high_sock + 1, &read_fds, &write_fds, &except_fds, NULL); // &timeout); //  NULL);

        switch (activity) {
            case -1:
                perror("select()");
                server_shutdown_properly(EXIT_FAILURE);

            case 0:
                fprintf(stderr, "select() returns 0.\n");
                // server_shutdown_properly(EXIT_FAILURE);

            default:

                if (FD_ISSET(http_sock_fd, &read_fds)) {
                    handle_new_connection(http_sock_fd, connection_list);
                }

                if (FD_ISSET(STDIN_FILENO, &except_fds)) {
                    SERVER_LOG("%s", "except_fds for stdin.")
                    server_shutdown_properly(EXIT_FAILURE);
                }

                if (FD_ISSET(http_sock_fd, &except_fds)) {
                    SERVER_LOG("%s", "Exception listen socket fd.")
                    server_shutdown_properly(EXIT_FAILURE);
                }

                for (i = 0; i < MAX_CLIENTS; ++i) {
                    has_read = false;
                    has_sent = false;
                    if (connection_list[i].socket != NO_SOCKET && FD_ISSET(connection_list[i].socket, &read_fds)) {
                        if (receive_from_peer(&connection_list[i], &echo_received_message) != EXIT_SUCCESS) {
                            close_client_connection(&connection_list[i]);
                            continue;
                        }else{
                            has_read = true;
                        }
                    }

                    if (connection_list[i].socket != NO_SOCKET &&
                        FD_ISSET(connection_list[i].socket, &write_fds)){ //&& !buf_empty(&connection_list[i].sending_buffer) )) {
                        int ret = send_to_peer(&connection_list[i]);
//                        SERVER_LOG("ret: %d", ret)
                        if (ret == EXIT_FAILURE) {
                            close_client_connection(&connection_list[i]);
                            continue;

                        } else if(ret == EXIT_SUCCESS) {
                            SERVER_LOG("%s", "send_to_peer returns EXIT_SUCCESS!")
                            has_sent = true;
                        } // otherwise if ret == NOTHING_TO_SEND, do nothing
                    }

                    if (connection_list[i].socket != NO_SOCKET && FD_ISSET(connection_list[i].socket, &except_fds)) {
                        SERVER_LOG("%s", "Exception client fd.")
                        close_client_connection(&connection_list[i]);
                        continue;
                    }

                    // handle HTTP
                    if(has_read || has_sent) {
                        int ret = handle_http(&connection_list[i]);
                        if(ret == CLOSE_CONN_IMMEDIATELY){
                            SERVER_LOG("Close connection %d", i)
                            close_client_connection(&connection_list[i]);

                        }else if(ret == CLOSE_CONN){
                            connection_list[i].close_conn = true;

                        }
                    }
                }
        }
    }
    return EXIT_SUCCESS;
}

void server_shutdown_properly(int code)
{
    int i;

    close(http_sock_fd);

    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (connection_list[i].socket != NO_SOCKET)
            delete_peer(&connection_list[i]); // de-allocate memory for each peer
    }
    printf("Shutdown server properly.\n");
    exit(code);
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
        if (connection_list[i].socket != NO_SOCKET) // !buf_empty(&connection_list[i].sending_buffer))
            FD_SET(connection_list[i].socket, write_fds);

    FD_ZERO(except_fds);
    FD_SET(STDIN_FILENO, except_fds);
    FD_SET(listen_sock, except_fds);
    for (i = 0; i < MAX_CLIENTS; ++i)
        if (connection_list[i].socket != NO_SOCKET)
            FD_SET(connection_list[i].socket, except_fds);

    return 0;
}

void handle_signal_action(int sig_number)
{
    if (sig_number == SIGINT) {
        printf("SIGINT was catched!\n");
        server_shutdown_properly(EXIT_SUCCESS);
    }
    else if (sig_number == SIGPIPE) { // there is nobody to receive your data
        printf("SIGPIPE was catched!\n");
        // server_shutdown_properly(EXIT_SUCCESS);
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

void init_server(int argc, char* argv[])
{
    /* Check usage */
    if (argc < 8) {
        fprintf(stdout, USAGE);
        http_port = 9999;
        WWW_DIR = DEFAULT_WWW_DIR;

    } else {
        /* Read parameters from command line arguments */
        http_port = atoi(argv[1]);   /* port param */
        https_port = atoi(argv[2]); /* https port param */
        // LOGFILE = argv[3];          /* log file param */
        // LOCKFILE = argv[4];         /* lock file param */
        WWW_DIR = argv[5];       /* www folder param */
        // CGI_scripts = argv[6];      /* cgi script param */
        // PRIVATE_KEY_FILE = argv[7]; /* private key file param */
        // CERT_FILE = argv[8];        /* certificate file param */
    }
    // check if resources folder exists
    if(!is_dir(WWW_DIR)){
        fprintf(stderr, "WWW path is wrong\n");
        exit(EXIT_FAILURE);
    }
}