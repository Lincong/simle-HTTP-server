//
// Created by Lincong Li on 1/4/18.
//

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/time.h>

#define RUN_AS_DAEMON true

#define IS_HTTP_SERVER true // if not a HTTP server, act like an echo server
#define MAX_CLIENTS 100
#define NO_SOCKET -1
#define SERVER_NAME "Liso/1.0"
#define LOG_FILE_NAME "log.txt"

/* Maximum bytes that can be send() or recv() via net by one call.
 * It's a good idea to test sending one byte by one.
 */
// #define ECHO_PORT 9999
#define BUF_SIZE 4096
#define BACKLOG  10
#define SOCK_REUSE 1

// parameters to turn on and off logging

// server communication logging

#define SERVER_LOG_ON true
#define COMM_LOG_ON true
#define HANDLER_LOG_ON false
#define HTTP_LOG_ON true
#define CGI_LOG_ON true

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// receiving from and sending to peer states
#define NOTHING_TO_SEND 2

// HTTP handler state
#define CLOSE_CONN 0 // close connection after the sending buffer becomes empty
#define CLOSE_CONN_IMMEDIATELY 1 // used when some error happened
#define KEEP_CONN  2
#define DO_CGI 3
#define DO_LAST_CGI 4


// HTTP protocol states
#define RECV_HEADER_STATE 1
#define RECV_BODY_STATE   2
#define GENERATE_HEADER_STATE 3
#define SEND_HEADER_STATE 4
#define SEND_BODY_STATE   5
#define FINISHED_STATE 6
#define CHECK_CGI 7
#define PROCESSING_CGI 8

#define DEFAULT_WWW_DIR "/Users/lincongli/Desktop/CSE124/src/www/"

typedef enum client_cgi_state {
    INVALID,
    READY_FOR_READ,
    READY_FOR_WRITE,
    WAITING_FOR_CGI,
    CGI_FOR_READ,
    CGI_FOR_WRITE
} client_cgi_state;

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr_in6 sockaddr_in6;
typedef struct sockaddr_storage sockaddr_storage;
typedef struct addrinfo addrinfo;

#endif //COMMON_H
