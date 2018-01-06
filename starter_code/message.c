//
// Created by Lincong Li on 1/4/18.
//

#include "message.h"
/*
  __  __
 |  \/  |
 | \  / | ___  ___ ___  __ _  __ _  ___
 | |\/| |/ _ \/ __/ __|/ _` |/ _` |/ _ \
 | |  | |  __/\__ \__ \ (_| | (_| |  __/
 |_|  |_|\___||___/___/\__,_|\__, |\___|
                              __/ |
                             |___/
*/

void print_message(char* msg, size_t len)
{
    int i;
    printf("Message: ");
    for(i = 0; i < len; i++) printf("%c", msg[i]);
    printf("\n");
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
    return 0;
}

int create_peer(peer_t *peer)
{
    reset_sending_buff(peer);
    reset_receiving_buff(peer);
    return 0;
}

void reset_sending_buff(peer_t *peer)
{
    peer->total_sending_bytes = 0;
    peer->curr_sending_bytes = 0;
    peer->is_sending = false;
    peer->sending_buf_size = sending_buf_size(peer);
    assert(peer->sending_buf_size == BUF_DATA_MAXSIZE);
}

void reset_receiving_buff(peer_t *peer)
{
    peer->start_receiving_byte = 0;
    peer->receiving_buf_size = receiving_buf_size(peer);
    assert(peer->receiving_buf_size == BUF_DATA_MAXSIZE);
    peer->available_receiving_buffer_byte = peer->receiving_buf_size;
}

size_t receiving_buf_size(peer_t *peer)
{
    return (size_t)(sizeof((peer->receiving_buffer).data) / sizeof(char));
}

size_t sending_buf_size(peer_t *peer)
{
    return (size_t)(sizeof((peer->sending_buffer).data) / sizeof(char));
}

int write_to_receiving_buffer(peer_t *peer, char data[], size_t num_bytes)
{
    if(num_bytes > peer->available_receiving_buffer_byte){
        perror("receiving buffer overflow");
        return EXIT_FAILURE;
    }
    peer->available_receiving_buffer_byte -= num_bytes;
    size_t buf_size = peer->receiving_buf_size;
    size_t end_byte = peer->start_receiving_byte + num_bytes;
    if(end_byte <= buf_size){
        memcpy((char*)(peer->receiving_buffer).data + peer->start_receiving_byte, data, num_bytes);
    }else{ // ring buffer needs to loop around
        size_t copied_size = buf_size - peer->start_receiving_byte;
        // fill up the rest of the ring buffer
        memcpy((char*)(peer->receiving_buffer).data + peer->start_receiving_byte, data, copied_size);

        // fill up the buffer from the beginning with the rest of bytes in data
        memcpy((char*)(peer->receiving_buffer).data, data + copied_size, num_bytes - copied_size);
    }
    end_byte %= buf_size;
    peer->start_receiving_byte = end_byte;
    return EXIT_SUCCESS;
}

size_t read_from_receiving_buffer(peer_t *peer, char data[], size_t bytes_read)
{
    size_t buf_size = peer->receiving_buf_size;
    size_t bytes_in_buff = buf_size - peer->available_receiving_buffer_byte;
    if(bytes_read > bytes_in_buff)
        bytes_read = bytes_in_buff;

    // calculate the start index of bytes to read
    size_t start_read_byte = peer->start_receiving_byte + peer->available_receiving_buffer_byte;

    // copy data
    int i;
    for(i = 0; i < bytes_read; i++)
        data[i] = *((char *) (peer->receiving_buffer).data + ((start_read_byte + i) % buf_size));

    if(bytes_read == bytes_in_buff)
        reset_receiving_buff(peer); // whole receiving buffer becomes available
    else
        peer->available_receiving_buffer_byte += bytes_read;
    return bytes_read;
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
int receive_from_peer(peer_t *peer, int (*message_handler)(peer_t *))
{
    printf("Ready for recv() from %s.\n", peer_get_addres_str(peer));

    size_t available_bytes;
    ssize_t received_count = 0; // has to be a signed size_t since recv might return negative value
    size_t received_total = 0;
    do {
        LOG("received_count ");
        printf("%ld\n", received_count);

        if (received_count > 0)
            message_handler(peer); // hopefully the handler consumes all bytes in the buffer

        // Count bytes to send.
        available_bytes = peer->available_receiving_buffer_byte;
        printf("Let's try to recv() %zd bytes...\n", available_bytes);
        // temporary buffer
        char data[available_bytes];

        // actual receiving
        received_count = recv(peer->socket, (char *)data, available_bytes, MSG_DONTWAIT);
        LOG("Received_count: ");
        printf("%ld\n", received_count);
        if (received_count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("peer is not ready right now, try again later.\n");
                break;
            }
            else {
                perror("recv() from peer error");
                return EXIT_FAILURE;
            }
        }
        // If recv() returns 0, it means that peer gracefully shutdown. Shutdown client.
        else if (received_count == 0) {
            printf("recv() 0 bytes. Peer gracefully shutdown.\n");
            return EXIT_FAILURE;
        }
        else if (received_count > 0) {
            if(write_to_receiving_buffer(peer, data, (size_t)received_count) != EXIT_SUCCESS)
                return EXIT_FAILURE;
            received_total += received_count;
            printf("recv() %zd bytes\n", received_count);
        }
    } while (received_count > 0);

    printf("Total recv()'ed %zu bytes.\n", received_total);
    reset_receiving_buff(peer);
    return EXIT_SUCCESS;
}

// once send_to_peer() is called, the sending buffer must have been emptyted before this function returns
int send_to_peer(peer_t *peer)
{
    if(!peer->is_sending) return 0; // not ready to be sent
    printf("Ready for send() to %s.\n", peer_get_addres_str(peer));
    ssize_t len_to_send;
    size_t send_total = 0; // total number of bytes sent
    ssize_t sent_count;
    do {
        // Count bytes to send.
        len_to_send = peer->total_sending_bytes - peer->curr_sending_bytes;
        if(len_to_send == 0){ // finish sending all bytes in the sending buffer
            reset_sending_buff(peer);
            LOG("successfully finish sending!");
            break;
        }

        if(len_to_send < 0) {
            perror("Something goes wrong! len_to_send < 0 ! This should never happen!!!");
            exit(-1);
        }

        if (len_to_send > MAX_SEND_SIZE) // if the sending_buffer is too big, send it chunk by chunk
            len_to_send = MAX_SEND_SIZE;

        printf("Let's try to send() %zd bytes...\n", len_to_send);

        // actual sending, should be blocking
        sent_count = send(peer->socket, (char *)&peer->sending_buffer + peer->curr_sending_bytes, len_to_send, 0);
        if (sent_count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("peer is not ready right now, try again later.\n");
                sent_count = 0;
            }
            else {
                perror("send() from peer error");
                return -1;
            }

        } else if (sent_count == 0) {
            printf("send()'ed 0 bytes. It seems that peer can't accept data right now. Try again later.\n");
            // break;

        } else if (sent_count > 0) {
            peer->curr_sending_bytes += sent_count;
            send_total += sent_count;
            printf("send()'ed %zd bytes.\n", sent_count);
        }
    } while (sent_count > 0);

    printf("Total send()'ed %zu bytes.\n", send_total);
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

    char client_ipv4_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ipv4_str, INET_ADDRSTRLEN);

    printf("Incoming connection from %s:%d.\n", client_ipv4_str, client_addr.sin_port);

    // reuse the next available peer
    int i;
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (connection_list[i].socket == NO_SOCKET) {
            connection_list[i].socket = new_client_sock;
            connection_list[i].addres = client_addr;
            return 0;
        }
    }

    printf("There is too much connections. Close new connection %s:%d.\n", client_ipv4_str, client_addr.sin_port);
    close(new_client_sock);
    return -1;
}

int close_client_connection(peer_t *client)
{
    printf("Close client socket for %s.\n", peer_get_addres_str(client));

    close(client->socket);
    client->socket = NO_SOCKET;
    // reset parameters for buffers
    reset_sending_buff(client);
    reset_receiving_buff(client);
    return 0;
}

int handle_received_message(peer_t *client)
{
    // try to print all bytes the receiving buffer
    size_t read_bytes_num = client->receiving_buf_size - client->available_receiving_buffer_byte;
    char msg[read_bytes_num];
    assert(read_bytes_num == read_from_receiving_buffer(client, msg, read_bytes_num));
    printf("Received message from client.\n");
    print_message(msg, read_bytes_num);
    return EXIT_SUCCESS;
}

int echo_received_message(peer_t *client)
{
    handle_received_message(client);
    // copy bytes from receiving_buffer to sending buffer
    // we can be sure that client is not sending at this point since the
    // server is single thread. If the client is in the sending function,
    // it won't return (unless errors occur) and the execution control
    // won't reach here


    // mark the client peer as ready to send
    client->is_sending = true;
    return EXIT_SUCCESS;
}
