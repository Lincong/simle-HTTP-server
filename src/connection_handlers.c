//
// Created by Lincong Li on 1/5/18.
//
#include "connection_handlers.h"
#include "logger.h"
#include "message.h"

// print all bytes the receiving buffer
int print_received_message(peer_t *client)
{

    size_t read_bytes_num = client->receiving_buffer.num_byte;

    // read all buffered bytes into a temporary buffer
    uint8_t msg[read_bytes_num];
    assert(read_bytes_num == read_from_receiving_buffer(client, msg, read_bytes_num));
    printf("Received message from client.\n");
    print_message(msg, read_bytes_num);
    return EXIT_SUCCESS;
}

// echo handler
int echo_received_message(peer_t *client)
{
//    // copy bytes from receiving_buffer to sending buffer
//    // we can be sure that client is not sending at this point since the
//    // server is single thread. If the client is in the sending function,
//    // it won't return (unless errors occur) and the execution control
//    // won't reach here
//
    // try to echo all bytes the receiving buffer
    COMM_LOG("%s", "In the receive handler")
    size_t echo_bytes_num = MIN(client->receiving_buffer.num_byte, buf_available(&client->sending_buffer));
    uint8_t msg[echo_bytes_num];
    assert(echo_bytes_num == read_from_receiving_buffer(client, msg, echo_bytes_num));

    // print data received
    HANDLER_LOG("%s", "Received message from client.")
    if(HANDLER_LOG_ON) print_message(msg, echo_bytes_num);

    if(write_to_sending_buffer(client, msg, echo_bytes_num) < 0){
        return EXIT_FAILURE;
    }
    // send bytes to client
    // if(send_to_peer(client) != EXIT_SUCCESS)
    //    return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
