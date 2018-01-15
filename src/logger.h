//
// Created by Lincong Li on 1/6/18.
//

#ifndef LOGGER_H
#define LOGGER_H

#include "common.h"

FILE *log_fd;

#define COMM_LOG(tag, msg) if(COMM_LOG_ON){char stringa[100];   \
                                sprintf(stringa, tag, msg);     \
                                fprintf(log_fd, "[COMM_LOG]: ");\
                                fprintf(log_fd, stringa);       \
                                fprintf(log_fd, "\n"); }

#define SERVER_LOG(tag, msg) if(SERVER_LOG_ON){char stringa[100];   \
                                sprintf(stringa, tag, msg);     \
                                fprintf(log_fd, "[SERVER_LOG]: ");\
                                fprintf(log_fd, stringa);       \
                                fprintf(log_fd, "\n"); }

#define HANDLER_LOG(tag, msg) if(HANDLER_LOG_ON){char stringa[100];   \
                                sprintf(stringa, tag, msg);     \
                                fprintf(log_fd, "[HANDLER_LOG]: ");\
                                fprintf(log_fd, stringa);       \
                                fprintf(log_fd, "\n"); }

#define HTTP_LOG(tag, msg) if(HTTP_LOG_ON){char stringa[100];   \
                                sprintf(stringa, tag, msg);     \
                                fprintf(log_fd, "[HTTP_LOG]: ");\
                                fprintf(log_fd, stringa);       \
                                fprintf(log_fd, "\n"); }

#endif //LOGGER_H
