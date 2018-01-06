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

#define BUF_DATA_MAXSIZE_IN_KB 1
#define BUF_DATA_MAXSIZE 1024 * BUF_DATA_MAXSIZE_IN_KB

// message --------------------------------------------------------------------

typedef struct {
    char data[BUF_DATA_MAXSIZE];
}  message_t;

// peer -----------------------------------------------------------------------

typedef struct {
    int socket;
    struct sockaddr_in addres;

    message_t sending_buffer;   // data in the outgoing buffer
    size_t sending_buf_size;
    size_t total_sending_bytes; // total bytes that needs to be sent out
    size_t curr_sending_bytes;  // currently the number of bytes that have been sent out
    bool is_sending;            // is the server sending bytes from this peer's buffer

    // data in the incoming buffer. It is used as a ring buffer
    // from the start_receiving_byte to start_receiving_byte + available_receiving_buffer_byte - 1
    // are the data in the buffer
    message_t receiving_buffer;
    size_t receiving_buf_size;
    size_t start_receiving_byte; // index to the next available byte in the receiving buffer
    size_t available_receiving_buffer_byte; // total number bytes the buffer can take in
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
// Assume the sending buffer is always empty when use this function
// Return EXIT_FAILURE if it fails
int write_to_sending_buffer(peer_t *peer, char data[], size_t num_bytes);

// return EXIT_FAILURE if it fails due to buffer overflow
int write_to_receiving_buffer(peer_t *peer, char data[], size_t num_bytes);
// If bytes_read > total number of bytes in the buffer, fill up data with
// all bytes. Assume the data array passed in has correctly allocated memory.
// Return the number of bytes that are filled in the buffer.
size_t read_from_receiving_buffer(peer_t *peer, char data[], size_t bytes_read);
// reset
void reset_sending_buff(peer_t *peer);
void reset_receiving_buff(peer_t *peer);

// connection -----------------------------------------------------------------

int handle_new_connection(int listen_sock, peer_t connection_list[]);
int close_client_connection(peer_t *client);
void print_message(char* msg, size_t len);

#endif //SERVER_MESSAGE_H
