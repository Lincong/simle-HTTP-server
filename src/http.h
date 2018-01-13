//
// Created by Lincong Li on 1/12/18.
//

#ifndef HTTP_H
#define HTTP_H

#include "common.h"
#include "message.h"

#define CL '\r'
#define RF '\n'
#define CLRF "\r\n"
#define SP " "
#define HTTP_VERSION  "HTTP/1.1"
#define COLON ":"
#define DEFAULT_INDEX_FILE "index.html"

#define HEADER_TERM_STATUS 4
#define PARSER_BUF_OVERFLOW 2
#define EMPTY_RECV_BUF 3

int handle_http(peer_t *peer);

#endif //HTTP_H
