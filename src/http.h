//
// Created by Lincong Li on 1/12/18.
//

#ifndef HTTP_H
#define HTTP_H

#include "common.h"
#include "message.h"

/*
  _     _____ _____ ____    _____  _    ____  _  __
 | | | |_   _|_   _|  _ \  |_   _|/ \  / ___|| |/ /
 | |_| | | |   | | | |_) |   | | / _ \ \___ \| ' /
 |  _  | | |   | | |  __/    | |/ ___ \ ___) | . \
 |_| |_| |_|   |_| |_|       |_/_/   \_\____/|_|\_\

*/

//typedef struct {
//    int state;
//    cbuf_t parse_buf;
//    cbuf_t response_buf; // might need to make it be dynamic buffer
//    FILE * fp;           // use fgetc() to get one byte at a time
//} http_task_t;
//
//http_task_t* create_http_task();
//void destroy_http_task(http_task_t* http_task);

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

//// queue node
//struct backlog_request {
//    http_task_t *http_task;
//    struct backlog_request *next;
//};
//
//// queue of backlog_request_t struct
//typedef struct {
//    struct backlog_request* front;
//    struct backlog_request* rear;
//    struct backlog_request* temp;
//    struct backlog_request* front1;
//    size_t size;
//} http_backlog_t;
//
//size_t http_backlog_size(http_backlog_t* backlog);
//http_task_t* peek_http_backlog(http_backlog_t* backlog);
//void enq_http_backlog(http_task_t* http_task, http_backlog_t* backlog);
//int deq_http_backlog(http_backlog_t* backlog);
//bool is_http_backlog_empty(http_backlog_t* backlog);
//void create_http_backlog(http_backlog_t* backlog);
//void destroy_http_backlog(http_backlog_t* backlog);
//
//
//typedef struct {
//    http_backlog_t* backlog;
//    cbuf_t parser_buffer;
//} http_agent_t;


/*
  _   _ _____ _____ ____    ____                            _     _                     _ _
 | | | |_   _|_   _|  _ \  |  _ \ ___  __ _ _   _  ___  ___| |_  | |__   __ _ _ __   __| | | ___  ___
 | |_| | | |   | | | |_) | | |_) / _ \/ _` | | | |/ _ \/ __| __| | '_ \ / _` | '_ \ / _` | |/ _ \/ __|
 |  _  | | |   | | |  __/  |  _ <  __/ (_| | |_| |  __/\__ \ |_  | | | | (_| | | | | (_| | |  __/\__ \
 |_| |_| |_|   |_| |_|     |_| \_\___|\__, |\__,_|\___||___/\__| |_| |_|\__,_|_| |_|\__,_|_|\___||___/
                                         |_|
*/

int handle_http(peer_t *peer);

#endif //HTTP_H
