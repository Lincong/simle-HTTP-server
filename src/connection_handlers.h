//
// Created by Lincong Li on 1/5/18.
//
#ifndef CONNECTION_HANDLERS_H
#define CONNECTION_HANDLERS_H

#include "message.h"

int print_received_message(peer_t *client);
int echo_received_message(peer_t *client);

#endif //CONNECTION_HANDLERS_H
