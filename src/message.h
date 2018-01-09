//
// Created by Lincong Li on 1/4/18.
//

#ifndef SERVER_MESSAGE_H
#define SERVER_MESSAGE_H

#include "common.h"

/* Maximum bytes that can be send() or recv() via net by one call.
 * It's a good idea to test sending one byte by one.
 */
#define MAX_SEND_SIZE 512

#define BUF_DATA_MAXSIZE_IN_KB 4
#define BUF_DATA_MAXSIZE 1024 * BUF_DATA_MAXSIZE_IN_KB

// buffer --------------------------------------------------------------------

typedef struct {
    uint8_t data[BUF_DATA_MAXSIZE];
    size_t head;
    size_t tail;
    size_t size; //of the buffer
    size_t num_byte; // current number of bytes
} cbuf_t;

int buf_reset(cbuf_t * cbuf);
size_t buf_available(cbuf_t* cbuf);
int buf_put(cbuf_t * cbuf, uint8_t data);
int buf_get(cbuf_t * cbuf, uint8_t * data);
int buf_putback(cbuf_t * cbuf); // to reverse the buf_get process

int buf_read(cbuf_t * cbuf, uint8_t * data, size_t num_bytes);
int buf_write(cbuf_t * cbuf, uint8_t * data, size_t num_bytes);
bool buf_empty(cbuf_t* cbuf);
bool buf_full(cbuf_t* cbuf);


// peer -----------------------------------------------------------------------

typedef struct {
    int socket;
    struct sockaddr_in addres;

    cbuf_t sending_buffer;   // data in the outgoing buffer
    cbuf_t receiving_buffer;
} peer_t;

int create_peer(peer_t *peer);
int delete_peer(peer_t *peer);
char *peer_get_addres_str(peer_t *peer);

// Receive message from peer and handle it with message_handler().
int receive_from_peer(peer_t *peer, int (*message_handler)(peer_t *));
// Send message from peer and empty its sending buffer
int send_to_peer(peer_t *peer);

// buffer input functions

// both sending and receiving buffers should have the same size
size_t receiving_buf_size(peer_t *peer);
size_t sending_buf_size(peer_t *peer);

// Return EXIT_FAILURE if it fails
int write_to_sending_buffer(peer_t *peer, uint8_t data[], size_t num_bytes);
int read_from_sending_buffer(peer_t *peer, uint8_t data[], size_t bytes_read);

// return EXIT_FAILURE if it fails due to buffer overflow
int write_to_receiving_buffer(peer_t *peer, uint8_t data[], size_t num_bytes);
// If bytes_read > total number of bytes in the buffer, fill up data with
// all bytes. Assume the data array passed in has correctly allocated memory.
// Return the number of bytes that are filled in the buffer.
int read_from_receiving_buffer(peer_t *peer, uint8_t data[], size_t bytes_read);
// reset
int reset_sending_buff(peer_t *peer);
int reset_receiving_buff(peer_t *peer);

// connection -----------------------------------------------------------------

int handle_new_connection(int listen_sock, peer_t connection_list[]);
int close_client_connection(peer_t *client);
void print_message(uint8_t* msg, size_t len);

#endif //SERVER_MESSAGE_H
