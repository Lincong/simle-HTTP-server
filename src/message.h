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

#define BUF_DATA_MAXSIZE_IN_KB 8
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
void free_buf(cbuf_t* cbuf);

// http_task --------------------------------------------------------------------

typedef struct {
    int state;
    uint8_t parse_buf[BUF_DATA_MAXSIZE];
    size_t parse_buf_idx;
    int header_term_token_status; // when it is 4, it means "\r\n\r\n" is matched, 3 means "\r\n\r" is matched and etc
    cbuf_t response_buf; // might need to make it be dynamic buffer
    int method_type;
    bool last_request;
    int response_code;
    // POST body parameters
    size_t body_bytes_num;
    char* post_body;
    int post_body_idx;

    FILE * fp;           // use fgetc() to get one byte at a time
} http_task_t;

http_task_t* create_http_task();
void destroy_http_task(http_task_t* http_task);
void reset_http_task(http_task_t* http_task);


// CGI executor -----------------------------------------------------------------

/* CGI */
#define CGI_ARGS_LEN 2
#define CGI_ENVP_LEN 22
#define CGI_ENVP_INFO_MAXLEN 1024

typedef struct host_and_port {
    char *host;
    int port;
} host_and_port;

/* CGI related parameters */
typedef struct CGI_param {
    char * filename;
    char* args[CGI_ARGS_LEN];
    char* envp[CGI_ENVP_LEN];
} CGI_param;

CGI_param *init_CGI_param();
void free_CGI_param(CGI_param*);


typedef struct CGI_executor {
    int clientfd;
    int stdin_pipe[2];    /* { write data --> stdin_pipe[1] } -> { stdin_pipe[0] --> stdin } */
    int stdout_pipe[2];   /* { read data <--  stdout_pipe[0] } <-- {stdout_pipe[1] <-- stdout } */
    cbuf_t* cgi_buffer;
    CGI_param* cgi_parameter;
} CGI_executor;

CGI_executor* init_CGI_executor();
void free_CGI_executor(CGI_executor *executor);
// peer -------------------------------------------------------------------------

typedef struct {
    int socket;
    struct sockaddr_in addres;
    http_task_t* http_task;
    CGI_executor* cgi_executor;
    cbuf_t sending_buffer;   // data in the outgoing buffer
    cbuf_t receiving_buffer;
    bool close_conn;
} peer_t;

int create_peer(peer_t *peer);
char *peer_get_addres_str(peer_t *peer);

int send_to_CGI_process(peer_t* client, int cgi_write_fd);
int pipe_from_CGI_process_to_client(peer_t* client, int cgi_read_fd);
// Receive message from peer and handle it with message_handler().
int receive_from_peer(peer_t *peer, int (*message_handler)(peer_t *));
// Send message from peer and empty its sending buffer
int send_to_peer(peer_t *peer);

// buffer input functions

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
