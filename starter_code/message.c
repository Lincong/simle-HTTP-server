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

int prepare_message(char *sender, char *data, message_t *message)
{
    sprintf(message->sender, "%s", sender);
    sprintf(message->data, "%s", data);
    return 0;
}

int print_message(message_t *message)
{
    printf("Message: \"%s: %s\"\n", message->sender, message->data);
    return 0;
}

/*
   _ __ ___   ___  ___ ___  __ _  __ _  ___    __ _ _   _  ___ _   _  ___
  | '_ ` _ \ / _ \/ __/ __|/ _` |/ _` |/ _ \  / _` | | | |/ _ \ | | |/ _ \
  | | | | | |  __/\__ \__ \ (_| | (_| |  __/ | (_| | |_| |  __/ |_| |  __/
  |_| |_| |_|\___||___/___/\__,_|\__, |\___|  \__, |\__,_|\___|\__,_|\___|
                                  __/ |          | |
                                 |___/           |_|
*/

int create_message_queue(int queue_size, message_queue_t *queue)
{
    queue->data = calloc(queue_size, sizeof(message_t));
    queue->size = queue_size;
    queue->current = 0;

    return 0;
}

void delete_message_queue(message_queue_t *queue)
{
    free(queue->data);
    queue->data = NULL;
}

int enqueue(message_queue_t *queue, message_t *message)
{
    if (queue->current == queue->size)
        return -1;

    memcpy(&queue->data[queue->current], message, sizeof(message_t));
    queue->current++;

    return 0;
}

int dequeue(message_queue_t *queue, message_t *message)
{
    if (queue->current == 0)
        return -1;

    memcpy(message, &queue->data[queue->current - 1], sizeof(message_t));
    queue->current--;

    return 0;
}

int dequeue_all(message_queue_t *queue)
{
    queue->current = 0;

    return 0;
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
    delete_message_queue(&peer->send_buffer);
    return 0;
}

int create_peer(peer_t *peer)
{
    create_message_queue(MAX_MESSAGES_BUFFER_SIZE, &peer->send_buffer);

    peer->current_sending_byte   = -1;
    peer->current_receiving_byte = 0;

    return 0;
}

char *peer_get_addres_str(peer_t *peer)
{
    static char ret[INET_ADDRSTRLEN + 10];
    char peer_ipv4_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer->addres.sin_addr, peer_ipv4_str, INET_ADDRSTRLEN);
    sprintf(ret, "%s:%d", peer_ipv4_str, peer->addres.sin_port);

    return ret;
}

int peer_add_to_send(peer_t *peer, message_t *message)
{
    return enqueue(&peer->send_buffer, message);
}

/* Receive message from peer and handle it with message_handler(). */
int receive_from_peer(peer_t *peer, int (*message_handler)(message_t *))
{
    printf("Ready for recv() from %s.\n", peer_get_addres_str(peer));

    size_t len_to_receive;
    ssize_t received_count;
    size_t received_total = 0;
    do {
        LOG("peer->current_receiving_byte: ");
        printf("%ld\n", peer->current_receiving_byte);

        LOG("sizeof(peer->receiving_buffer):");
        printf("%ld\n", sizeof(peer->receiving_buffer));
        // Is completely received?
        if (peer->current_receiving_byte >= sizeof(peer->receiving_buffer)) {
            message_handler(&peer->receiving_buffer);
            peer->current_receiving_byte = 0;
        }

        // Count bytes to send.
        len_to_receive = sizeof(peer->receiving_buffer) - peer->current_receiving_byte;
        if (len_to_receive > MAX_SEND_SIZE)
            len_to_receive = MAX_SEND_SIZE;

        printf("Let's try to recv() %zd bytes... ", len_to_receive);
        received_count = recv(peer->socket, (char *)&peer->receiving_buffer + peer->current_receiving_byte, len_to_receive, MSG_DONTWAIT);
        LOG("Received_count: ");
        printf("%ld\n", received_count);
        if (received_count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("peer is not ready right now, try again later.\n");
            }
            else {
                perror("recv() from peer error");
                return -1;
            }
        }
        else if (received_count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            break;
        }
        // If recv() returns 0, it means that peer gracefully shutdown. Shutdown client.
        else if (received_count == 0) {
            printf("recv() 0 bytes. Peer gracefully shutdown.\n");
            return -1;
        }
        else if (received_count > 0) {
            peer->current_receiving_byte += received_count;
            received_total += received_count;
            printf("recv() %zd bytes\n", received_count);
        }
    } while (received_count > 0);

    printf("Total recv()'ed %zu bytes.\n", received_total);
    return 0;
}

int send_to_peer(peer_t *peer)
{
    printf("Ready for send() to %s.\n", peer_get_addres_str(peer));

    size_t len_to_send;
    ssize_t send_count;
    size_t send_total = 0;
    do {
        // If sending message has completely sent and there are messages in queue, why not send them?
        if (peer->current_sending_byte < 0 || peer->current_sending_byte >= sizeof(peer->sending_buffer)) {
            printf("There is no pending to send() message, maybe we can find one in queue... ");
            if (dequeue(&peer->send_buffer, &peer->sending_buffer) != 0) {
                peer->current_sending_byte = -1;
                printf("No, there is nothing to send() anymore.\n");
                break;
            }
            printf("Yes, pop and send() one of them.\n");
            peer->current_sending_byte = 0;
        }

        // Count bytes to send.
        len_to_send = sizeof(peer->sending_buffer) - peer->current_sending_byte;
        if (len_to_send > MAX_SEND_SIZE)
            len_to_send = MAX_SEND_SIZE;

        printf("Let's try to send() %zd bytes... ", len_to_send);
        send_count = send(peer->socket, (char *)&peer->sending_buffer + peer->current_sending_byte, len_to_send, 0);
        if (send_count < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("peer is not ready right now, try again later.\n");
                break;
            }
            else {
                perror("send() from peer error");
                return -1;
            }
        }
        // we have read as many as possible
        // else if (send_count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        //    break;
        // }
        else if (send_count == 0) {
            printf("send()'ed 0 bytes. It seems that peer can't accept data right now. Try again later.\n");
            break;
        }
        else if (send_count > 0) {
            peer->current_sending_byte += send_count;
            send_total += send_count;
            printf("send()'ed %zd bytes.\n", send_count);
        }
    } while (send_count > 0);

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

    int i;
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (connection_list[i].socket == NO_SOCKET) {
            connection_list[i].socket = new_client_sock;
            connection_list[i].addres = client_addr;
            connection_list[i].current_sending_byte   = -1;
            connection_list[i].current_receiving_byte = 0;
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
    dequeue_all(&client->send_buffer);
    client->current_sending_byte   = -1;
    client->current_receiving_byte = 0;
    return 0;
}

/* Reads from stdin and create new message. This message enqueues to send queueu. */
//int handle_read_from_stdin()
//{
//    char read_buffer[DATA_MAXSIZE]; // buffer for stdin
//    if (read_from_stdin(read_buffer, DATA_MAXSIZE) != 0)
//        return -1;
//
//    // Create new message and enqueue it.
//    message_t new_message;
//    prepare_message(SERVER_NAME, read_buffer, &new_message);
//    print_message(&new_message);
//
//    /* enqueue message for all clients */
//    int i;
//    for (i = 0; i < MAX_CLIENTS; ++i) {
//        if (connection_list[i].socket != NO_SOCKET) {
//            if (peer_add_to_send(&connection_list[i], &new_message) != 0) {
//                printf("Send buffer was overflowed, we lost this message!\n");
//                continue;
//            }
//            printf("New message to send was enqueued right now.\n");
//        }
//    }
//
//    return 0;
//}

int handle_received_message(message_t *message)
{
    printf("Received message from client.\n");
    print_message(message);
    return 0;
}
