//
// Created by Lincong Li on 1/2/18.
//

#ifndef SERVER_UTILITY_H
#define SERVER_UTILITY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum bytes that can be send() or recv() via net by one call.
 * It's a good idea to test sending one byte by one.
 */
#define MAX_SEND_SIZE 100

/* Size of send queue (messages). */
// #define MAX_MESSAGES_BUFFER_SIZE 10

// #define SENDER_MAXSIZE 128
#define DATA_MAXSIZE 512

#define ECHO_PORT 9999
#define BUF_SIZE 4096
#define BACKLOG  10
#define SOCK_REUSE 1

#endif
