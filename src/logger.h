//
// Created by Lincong Li on 1/6/18.
//

#ifndef LOGGER_H
#define LOGGER_H

#include "common.h"

#define COMM_LOG(tag, msg) if(COMM_LOG_ON){ printf((tag), (msg)); printf("\n"); }

#define SERVER_LOG(tag, msg) if(SERVER_LOG_ON){ printf((tag), (msg)); printf("\n"); }

#define HANDLER_LOG(tag, msg) if(HANDLER_LOG_ON){ printf((tag), (msg)); printf("\n"); }

#define HTTP_LOG(tag, msg) if(HTTP_LOG_ON){ printf((tag), (msg)); printf("\n"); }

#endif //LOGGER_H
