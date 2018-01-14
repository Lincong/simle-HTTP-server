//
// Created by Lincong Li on 1/4/18.
//

#include "message.h"
#include "logger.h"
/*
 _            __  __
| |          / _|/ _|
| |__  _   _| |_| |_ ___ _ __
| '_ \| | | |  _|  _/ _ \ '__|
| |_) | |_| | | | ||  __/ |
|_.__/ \__,_|_| |_| \___|_|

*/

int buf_reset(cbuf_t* cbuf)
{
    int ret = EXIT_FAILURE;
    if(cbuf)
    {
        cbuf->head = 0;
        cbuf->tail = 0;
        cbuf->size = BUF_DATA_MAXSIZE;
        cbuf->num_byte = 0;
        ret = EXIT_SUCCESS;
    }
    return ret;
}

size_t buf_available(cbuf_t* cbuf)
{
    size_t ret = cbuf->size - cbuf->num_byte;
    assert(ret >= 0);
    return ret;
}

bool buf_empty(cbuf_t* cbuf)
{
    return (buf_available(cbuf) == cbuf->size);
}

bool buf_full(cbuf_t* cbuf)
{
    return (buf_available(cbuf) == 0);
}

// return one byte at a time
int buf_put(cbuf_t * cbuf, uint8_t data)
{
    int ret = EXIT_FAILURE;

    if(cbuf && !buf_full(cbuf))
    {
        cbuf->data[cbuf->head] = data;
        cbuf->head = (cbuf->head + 1) % cbuf->size;

        cbuf->num_byte++;
        ret = EXIT_SUCCESS;
    }

    return ret;
}

int buf_get(cbuf_t * cbuf, uint8_t * data)
{
    int ret = EXIT_FAILURE;

    if(cbuf && data && !buf_empty(cbuf))
    {
        *data = cbuf->data[cbuf->tail];
        cbuf->tail = (cbuf->tail + 1) % cbuf->size;
        cbuf->num_byte--;
        ret = EXIT_SUCCESS;
    }

    return ret;
}

int buf_putback(cbuf_t * cbuf)
{
    int ret = EXIT_FAILURE;
    if(cbuf && !buf_full(cbuf))
    {
        if(cbuf->tail == 0)
            cbuf->tail = cbuf->size;
        cbuf->tail--;
        cbuf->num_byte++;
        ret = EXIT_SUCCESS;
    }
    return ret;
}

int buf_read(cbuf_t * cbuf, uint8_t * data, size_t num_bytes)
{
    // read at most all bytes in the buffer
    if(num_bytes > cbuf->num_byte)
        num_bytes = cbuf->num_byte;

    int i;
    for(i = 0; i < num_bytes; i++){
        if(buf_get(cbuf, &data[i]) != EXIT_SUCCESS)
            return EXIT_FAILURE;
    }
    return (int)num_bytes;
}

int buf_write(cbuf_t * cbuf, uint8_t * data, size_t num_bytes)
{
    // write at most all bytes in data into the buffer
    if(num_bytes > buf_available(cbuf)) {
        num_bytes = buf_available(cbuf);
        printf("buffer overflow!\n");
        return -1;
    }

    int i;
    for(i = 0; i < num_bytes; i++){
        if(buf_put(cbuf, data[i]) != EXIT_SUCCESS) {
            return -1;
        }
    }
    return (int)num_bytes;
}

void print_message(uint8_t* msg, size_t len)
{
    int i;
    printf("Message: ");
    for(i = 0; i < len; i++) printf("%c", (char)msg[i]);
    printf("\n");
}


/*
  _     _____ _____ ____    _____  _    ____  _  __
 | | | |_   _|_   _|  _ \  |_   _|/ \  / ___|| |/ /
 | |_| | | |   | | | |_) |   | | / _ \ \___ \| ' /
 |  _  | | |   | | |  __/    | |/ ___ \ ___) | . \
 |_| |_| |_|   |_| |_|       |_/_/   \_\____/|_|\_\

*/

http_task_t* create_http_task()
{
    http_task_t* new_task = (http_task_t*) malloc(1*sizeof(http_task_t));
    new_task->state = RECV_HEADER_STATE; // initial state
    new_task->parse_buf_idx = 0;
    new_task->header_term_token_status = 0;
    buf_reset(&new_task->response_buf);
    return new_task;
}

void destroy_http_task(http_task_t* http_task)
{
    if(http_task != NULL)
        free(http_task);
}


/*
 | '_ \ / _ \/ _ \ '__|
 | |_) |  __/  __/ |
 | .__/ \___|\___|_|
 | |
 |_|
*/
int delete_peer(peer_t *peer)
{
    close(peer->socket);
    destroy_http_task(peer->http_task);
    return 0;
}

int create_peer(peer_t *peer)
{
    peer->http_task = create_http_task();
    peer->close_conn = false;
    reset_sending_buff(peer);
    reset_receiving_buff(peer);
    return 0;
}

int reset_sending_buff(peer_t *peer)
{
    return buf_reset(&peer->sending_buffer);
}

int reset_receiving_buff(peer_t *peer)
{
    return buf_reset(&peer->receiving_buffer);
}


// sending buffer I/O

// returns the number of bytes that are successfully written to the sending buffer
int write_to_sending_buffer(peer_t *peer, uint8_t data[], size_t num_bytes)
{
    return buf_write(&peer->sending_buffer, data, num_bytes);
}

int read_from_sending_buffer(peer_t *peer, uint8_t  data[], size_t bytes_read)
{
    return buf_read(&peer->sending_buffer, data, bytes_read);
}

// receiving buffer I/O

int write_to_receiving_buffer(peer_t *peer, uint8_t data[], size_t num_bytes)
{
    return buf_write(&peer->receiving_buffer, data, num_bytes);
}

int read_from_receiving_buffer(peer_t *peer, uint8_t data[], size_t bytes_read)
{
    return buf_read(&peer->receiving_buffer, data, bytes_read);
}

char *peer_get_addres_str(peer_t *peer)
{
    static char ret[INET_ADDRSTRLEN + 10];
    char peer_ipv4_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer->addres.sin_addr, peer_ipv4_str, INET_ADDRSTRLEN);
    sprintf(ret, "%s:%d", peer_ipv4_str, peer->addres.sin_port);
    return ret;
}

// Receive message from peer and handle it with message_handler(). The message handler is responsible
// for adjusting parameters of the receiving buffer
// Depends on how much buffer available, receive only once
int receive_from_peer(peer_t *peer, int (*message_handler)(peer_t *))
{
    COMM_LOG("Ready for recv() from %s.", peer_get_addres_str(peer))

    size_t available_bytes;
    ssize_t received_count = 0; // has to be a signed size_t since recv might return negative value

    // Count bytes to receive.
    available_bytes = buf_available(&peer->receiving_buffer);
    COMM_LOG("Let's try to recv() %zd bytes...", available_bytes)
    // temporary buffer to receive bytes from the sys call
    uint8_t data[available_bytes];
    // actual receiving
    received_count = recv(peer->socket, (uint8_t *)data, available_bytes, MSG_DONTWAIT);
    COMM_LOG("%s", "Actual received_count: ")
    COMM_LOG("%ld", received_count)
    if (received_count < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            COMM_LOG("%s", "peer is not ready right now, try again later.")
            return EXIT_SUCCESS;
        }
        else {
            perror("recv() from peer error");
            return EXIT_FAILURE;
        }
    }// If recv() returns 0, it means that peer gracefully shutdown. Shutdown client.
    else if (received_count == 0) {
        COMM_LOG("%s", "recv() 0 bytes. Peer gracefully shutdown.")
        return EXIT_FAILURE;
    }
    else if (received_count > 0) {
        if(write_to_receiving_buffer(peer, data, (size_t)received_count) == EXIT_FAILURE) {
            COMM_LOG("%s", "Write to receiving buffer failed!")
            return EXIT_FAILURE;
        }

        if(!IS_HTTP_SERVER) {
            // send the receiving bytes to client
            if (message_handler(peer) != EXIT_SUCCESS)
                return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

// once send_to_peer() is called, at most all bytes in the sending buffer are sent
int send_to_peer(peer_t *peer)
{
    // COMM_LOG("Ready for send() to %s.", peer_get_addres_str(peer))
    ssize_t buf_bytes_cnt;   // bytes that need to be sent
    ssize_t sent_bytes_cnt;    // bytes that have been sent at each iteration

    // Count bytes to send.
    cbuf_t* sending_buf = &peer->sending_buffer;
    buf_bytes_cnt = sending_buf->num_byte;
    if(buf_bytes_cnt == 0) {
        // COMM_LOG("%s", "no data in sending buffer")
        if(peer->close_conn) {
            // COMM_LOG("%s", "return EXIT_FAILURE")
            return EXIT_FAILURE; // tell the caller to close the connection
        }
        return NOTHING_TO_SEND;
    }
    COMM_LOG("# of bytes in sending_buffer is: %d", buf_bytes_cnt)
    uint8_t data[buf_bytes_cnt];
    buf_bytes_cnt = read_from_sending_buffer(peer, data, buf_bytes_cnt);
    COMM_LOG("Let's try to send() %zd bytes...", buf_bytes_cnt)

    // actual sending, should be non-blocking
    sent_bytes_cnt = send(peer->socket, (uint8_t *)data, buf_bytes_cnt, 0);
    COMM_LOG("%s", "Done sending!")

    if (sent_bytes_cnt < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            COMM_LOG("%s", "peer is not ready right now, try again later.")
        } else {
            COMM_LOG("%s", "send() from peer error")
            return EXIT_FAILURE;
        }

    } else if (sent_bytes_cnt > 0) {
        COMM_LOG("send()'ed %zd bytes.", sent_bytes_cnt)

    } else if (sent_bytes_cnt == 0) {
        COMM_LOG("%s", "send()'ed 0 bytes. It seems that peer can't accept data right now. Try again later.")
    }
    COMM_LOG("# of bytes in sending_buffer after sending is: %d", (int)sending_buf->num_byte)
    sent_bytes_cnt = (sent_bytes_cnt < 0 ? 0 : sent_bytes_cnt);
    // write the unsent bytes back to the sending buffer
    while(sent_bytes_cnt < buf_bytes_cnt){
        assert( buf_putback(sending_buf) != EXIT_FAILURE);
        sent_bytes_cnt++;
    }

    if(peer->close_conn && buf_empty(&(peer->sending_buffer))) {
        COMM_LOG("%s", "send_to_peer returns EXIT_FAILURE")
        return EXIT_FAILURE; // tell the caller to close the connection
    }
    COMM_LOG("%s", "send_to_peer returns EXIT_SUCCESS")
    return 0;
}

/*
                                 | | (_)
   ___ ___  _ __  _ __   ___  ___| |_ _  ___  _ __
  / __/ _ \| '_ \| '_ \ / _ \/ __| __| |/ _ \| '_ \
 | (_| (_) | | | | | | |  __/ (__| |_| | (_) | | | |
  \___\___/|_| |_|_| |_|\___|\___|\__|_|\___/|_| |_|
*/

int handle_new_connection(int listen_sock, peer_t connection_list[])
{
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t client_len = sizeof(client_addr);
    int new_client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
    if (new_client_sock < 0) {
        perror("accept()");
        return -1;
    }
    // set the socket to be non-blocking
    int flags = fcntl(new_client_sock, F_GETFL, 0);
    if (flags < 0) return false;
    flags = flags | O_NONBLOCK;
    assert(fcntl(new_client_sock, F_SETFL, flags) == EXIT_SUCCESS);

    char client_ipv4_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ipv4_str, INET_ADDRSTRLEN);

    COMM_LOG("%s", "Incoming connection from:")
    COMM_LOG("IP: %s", client_ipv4_str)
    COMM_LOG("Port: %d", client_addr.sin_port)
    // reuse the next available peer
    int i;
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (connection_list[i].socket == NO_SOCKET) {
            connection_list[i].socket = new_client_sock;
            connection_list[i].addres = client_addr;
            connection_list[i].close_conn = false;
            connection_list[i].http_task = create_http_task();
            COMM_LOG("%s", "Done initializing connection")
            return 0;
        }
    }

    COMM_LOG("%s", "There is too much connections. Close")
    COMM_LOG("IP: %s", client_ipv4_str)
    COMM_LOG("Port: %d", client_addr.sin_port)
    close(new_client_sock);
    return -1;
}

int close_client_connection(peer_t *client)
{
    COMM_LOG("Close client socket for %s.", peer_get_addres_str(client))

    close(client->socket);
    client->socket = NO_SOCKET;
    client->close_conn = false; // may not be necessary
    // reset parameters for buffers
    reset_sending_buff(client);
    reset_receiving_buff(client);
    destroy_http_task(client->http_task);
    return 0;
}
