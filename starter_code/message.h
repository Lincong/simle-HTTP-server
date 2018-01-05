//
// Created by Lincong Li on 1/4/18.
//

#ifndef SERVER_MESSAGE_H
#define SERVER_MESSAGE_H

#include "common.h"

/* Maximum bytes that can be send() or recv() via net by one call.
 * It's a good idea to test sending one byte by one.
 */
#define MAX_SEND_SIZE 100

/* Size of send queue (messages). */
#define MAX_MESSAGES_BUFFER_SIZE 10

#define DATA_MAXSIZE_IN_KB 1
#define DATA_MAXSIZE 1024 * DATA_MAXSIZE_IN_KB

// message --------------------------------------------------------------------

typedef struct {
    char data[DATA_MAXSIZE];
    int msg_size; // how many bytes are in this message (max == DATA_MAXSIZE)
}  message_t;

int print_message(message_t *message);

// message queue --------------------------------------------------------------

typedef struct {
    int size;
    message_t *data; // array of messages
    int current; // current index
} message_queue_t;

int create_message_queue(int queue_size, message_queue_t *queue);
void delete_message_queue(message_queue_t *queue);
int enqueue(message_queue_t *queue, message_t *message);
int dequeue(message_queue_t *queue, message_t *message);
int dequeue_all(message_queue_t *queue);

// peer -----------------------------------------------------------------------

typedef struct {
    int socket;
    struct sockaddr_in addres;

    /* Messages that waiting for send. */
    message_queue_t send_buffer;

    /* Buffered sending message.
     *
     * In case we doesn't send whole message per one call send().
     * And current_sending_byte is a pointer to the part of data that will be send next call.
     */
    message_t sending_buffer;
    int current_sending_byte;
    bool ready_to_send;
    /* The same for the receiving message. */
    message_t receiving_buffer;
    size_t current_receiving_byte;
} peer_t;

int create_peer(peer_t *peer);
int delete_peer(peer_t *peer);
char *peer_get_addres_str(peer_t *peer);
int peer_add_to_send(peer_t *peer, message_t *message);
/* Receive message from peer and handle it with message_handler(). */
int receive_from_peer(peer_t *peer, int (*message_handler)(message_t *, ssize_t));
int send_to_peer(peer_t *peer);

// connection -----------------------------------------------------------------

int handle_new_connection(int listen_sock, peer_t connection_list[]);
int close_client_connection(peer_t *client);
int handle_received_message(message_t *message, ssize_t received_num);
//int handle_read_from_stdin();

#endif //SERVER_MESSAGE_H
