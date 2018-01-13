//
// Created by Lincong Li on 1/12/18.
//

#ifndef HTTP_H
#define HTTP_H

#include "common.h"
#include "message.h"

#define CLOSE_CONN 1
#define KEEP_CONN  2

// HTTP protocol states
#define RECV_HEADER_STATE 1
#define RECV_BODY_STATE   2
#define SEND_HEADER_STATE 3
#define SEND_BODY_STATE   4

typedef struct {
    int state;
    cbuf_t parser_buf;
    FILE * fp; // use fgetc() to get one byte at a time
} http_task_t;


/*
 _                _    _
| |              | |  | |
| |__   __ _  ___| | _| | ___   __ _    __ _ _   _  ___ _   _  ___
| '_ \ / _` |/ __| |/ / |/ _ \ / _` |  / _` | | | |/ _ \ | | |/ _ \
| |_) | (_| | (__|   <| | (_) | (_| | | (_| | |_| |  __/ |_| |  __/
|_.__/ \__,_|\___|_|\_\_|\___/ \__, |  \__, |\__,_|\___|\__,_|\___|
                                __/ |     | |
                               |___/      |_|
*/

// queue node
typedef struct {
    http_task_t* http_task;
    requests_backlog_t *next;
} backlog_request_t;

// queue of backlog_request_t struct
typedef struct {
    backlog_request_t* front;
    backlog_request_t* rear;
    backlog_request_t* temp;
    backlog_request_t* front1;
    size_t size;
} backlog_t;

size_t backlog_size(backlog_t* backlog);
http_task_t* peek(backlog_t* backlog);
void enq(http_task_t* http_task, backlog_t* backlog);
void deq(backlog_t* backlog);
void empty(backlog_t* backlog);
void create(backlog_t* backlog);

typedef struct {

} http_agent_t;


typedef struct {
    cbuf_t parser_buffer;

} http_agent_t;

int handle_http(peer_t *peer);

#endif //HTTP_H
