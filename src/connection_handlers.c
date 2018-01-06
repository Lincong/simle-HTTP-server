//
// Created by Lincong Li on 1/5/18.
//
#include "connection_handlers.h"
#include "logger.h"

int print_received_message(peer_t *client)
{
    // try to print all bytes the receiving buffer
    size_t read_bytes_num = client->receiving_buf_size - client->available_receiving_buffer_byte;
    char msg[read_bytes_num];
    assert(read_bytes_num == read_from_receiving_buffer(client, msg, read_bytes_num));
    printf("Received message from client.\n");
    print_message(msg, read_bytes_num);
    return EXIT_SUCCESS;
}

// echo handler
int echo_received_message(peer_t *client)
{
    // copy bytes from receiving_buffer to sending buffer
    // we can be sure that client is not sending at this point since the
    // server is single thread. If the client is in the sending function,
    // it won't return (unless errors occur) and the execution control
    // won't reach here

    // try to print all bytes the receiving buffer
    size_t read_bytes_num = client->receiving_buf_size - client->available_receiving_buffer_byte;
    char msg[read_bytes_num];
    assert(read_bytes_num == read_from_receiving_buffer(client, msg, read_bytes_num));

    // print data received
    HANDLER_LOG("%s", "Received message from client.")
    if(HANDLER_LOG_ON) print_message(msg, read_bytes_num);

    write_to_sending_buffer(client, msg, read_bytes_num);
    // mark the client peer as ready to send
    client->is_sending = true;
    // send bytes to client
    if(send_to_peer(client) != EXIT_SUCCESS)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
