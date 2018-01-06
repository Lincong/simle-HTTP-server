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

#define LOG(msg) printf(msg); printf("\n");

#endif //COMMON_H
