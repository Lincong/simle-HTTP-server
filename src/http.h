//
// Created by Lincong Li on 1/12/18.
//

#ifndef HTTP_H
#define HTTP_H

#include "common.h"
#include "message.h"

#define CLOSE_CONN 1
#define KEEP_CONN  2

int handle_http(peer_t *peer);

#endif //HTTP_H
