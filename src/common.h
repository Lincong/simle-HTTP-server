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

#define IS_HTTP_SERVER false // if not a HTTP server, act like an echo server
#define MAX_CLIENTS 100
#define NO_SOCKET -1
#define SERVER_NAME "server"

/* Maximum bytes that can be send() or recv() via net by one call.
 * It's a good idea to test sending one byte by one.
 */
#define ECHO_PORT 9999
#define BUF_SIZE 4096
#define BACKLOG  10
#define SOCK_REUSE 1
#define CLOSE_CONN 1
#define KEEP_CONN 1

// parameters to turn on and off logging

// server communication logging

#define SERVER_LOG_ON true
#define COMM_LOG_ON true
#define HANDLER_LOG_ON false
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#endif //COMMON_H
