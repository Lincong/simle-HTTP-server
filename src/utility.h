//
// Created by Lincong Li on 1/2/18.
//

#ifndef SERVER_UTILITY_H
#define SERVER_UTILITY_H

#include "common.h"

int start_listen_socket(int backlog_num, int reuse, int* sock_fd);
int close_socket(int sock);

#endif
