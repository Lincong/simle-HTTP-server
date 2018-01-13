//
// Created by Lincong Li on 1/12/18.
//

#include "http.h"
#include "message.h"
#include "logger.h"
#include "parser/parse.h"

int read_header_data(peer_t *peer);
void print_req(Request* request);


/*
  _   _ _____ _____ ____    ____                            _     _                     _ _
 | | | |_   _|_   _|  _ \  |  _ \ ___  __ _ _   _  ___  ___| |_  | |__   __ _ _ __   __| | | ___  ___
 | |_| | | |   | | | |_) | | |_) / _ \/ _` | | | |/ _ \/ __| __| | '_ \ / _` | '_ \ / _` | |/ _ \/ __|
 |  _  | | |   | | |  __/  |  _ <  __/ (_| | |_| |  __/\__ \ |_  | | | | (_| | | | | (_| | |  __/\__ \
 |_| |_| |_|   |_| |_|     |_| \_\___|\__, |\__,_|\___||___/\__| |_| |_|\__,_|_| |_|\__,_|_|\___||___/
                                         |_|
*/

char *WWW_FOLDER;

///* Defineds tokens */
//char *clrf = "\r\n";
//char *sp = " ";
//char *http_version = "HTTP/1.1";
//char *colon = ":";
//
///* Constants string values */
//char *default_index_file = "index.html";
//char *server_str = "Liso/1.0";

int handle_http(peer_t *peer)
{
    /*
       // process input part
  1. read all data from the recv buffer and process it
  2. generate Request
  3. Use Request object to generate Response object
  4. Put the response object in the back_log (queue)

  // generate output part
  5. check the state of the current "http_task"
  6. do corresponding actions
  7. if the current "http_task" is finished. Get another one from the queue and repeat from step 5 until the send buffer is filled up
  8. return
     */
    HTTP_LOG("%s", "In HTTP handler!")
    http_task_t* curr_task = peer->http_task;
    if(curr_task == NULL){
        fprintf(stderr, "When handle_http() is called, http_task should have been allocated");
        exit(EXIT_FAILURE);
    }

    // RECV_HEADER_STATE -> RECV_BODY_STATE -> GENERATE_HEADER_STATE ->
    // SEND_HEADER_STATE -> SEND_BODY_STATE -> FINISHED_STATE
    while(curr_task->state != FINISHED_STATE){
        if(curr_task->state == RECV_HEADER_STATE){
            // read bytes from receiving buffer into parser buffer
            HTTP_LOG("%s", "Before read_header_data")
            int ret = read_header_data(peer);
            if(ret == PARSER_BUF_OVERFLOW) {
                return CLOSE_CONN_IMMEDIATELY; // invalid header request

            } else if(ret == EMPTY_RECV_BUF) {
                return KEEP_CONN;

            }
            Request *request = parse((char*)curr_task->parse_buf, curr_task->parse_buf_idx, NULL);
            if(request == NULL) {
                HTTP_LOG("%s", "parse() returns NULL")
                exit(0);
                return CLOSE_CONN_IMMEDIATELY; // invalid header request
            }
            if(HTTP_LOG_ON)
                print_req(request);

            free(request->headers);
            free(request);
            return CLOSE_CONN_IMMEDIATELY; // test

            curr_task->state = RECV_BODY_STATE;

        }else if(curr_task->state == RECV_BODY_STATE){

            curr_task->state = GENERATE_HEADER_STATE;

        }else if(curr_task->state == GENERATE_HEADER_STATE){

            curr_task->state = SEND_HEADER_STATE;

        }else if(curr_task->state == SEND_HEADER_STATE){

            curr_task->state = SEND_BODY_STATE;

        }else if(curr_task->state == SEND_BODY_STATE){

            curr_task->state = FINISHED_STATE;
        }else{
            fprintf(stderr, "Shouldn't get to this unknown state");
            exit(EXIT_FAILURE);
        }
    }

    return CLOSE_CONN;
}


// HTTP request handler helpers

// read bytes from receiving buffer of the client to parser buffer of it
// return EMPTY_RECV_BUF | EXIT_SUCCESS | PARSER_BUF_OVERFLOW
int read_header_data(peer_t *peer)
{
    assert(peer != NULL);
    http_task_t* curr_task = peer->http_task;
    assert(curr_task != NULL);
    uint8_t * parse_buf = curr_task->parse_buf;
    char* terminator = "\r\n\r\n";

    uint8_t data;
    while(curr_task->header_term_token_status != HEADER_TERM_STATUS){
        if(buf_empty(&peer->receiving_buffer)) // no more to read from the receiving buffer
            return EMPTY_RECV_BUF;
        buf_get(&peer->receiving_buffer, &data);

        if(curr_task->header_term_token_status == 0){
            if(data == terminator[0])
                curr_task->header_term_token_status = 1;
            // otherwise, state is still 0

        } else if(curr_task->header_term_token_status == 1) {
            if(data == terminator[1]) { // second character is '\n'
                curr_task->header_term_token_status = 2; // move to next state

            } else if(data != terminator[0]){
               curr_task->header_term_token_status = 0; // start over
            }

        } else if(curr_task->header_term_token_status == 2) {
            if(data == terminator[2]) { // second character is '\r'
                curr_task->header_term_token_status = 3; // move to next state

            } else if(data == terminator[0]) {
                curr_task->header_term_token_status = 1; // go back to the second state

            } else {
                curr_task->header_term_token_status = 0; // start over

            }

        } else if(curr_task->header_term_token_status == 3) {
            if(data == terminator[3]) { // second character is '\n'
                curr_task->header_term_token_status = HEADER_TERM_STATUS; // move to next state

            } else if(data == terminator[0]) {
                curr_task->header_term_token_status = 1; // go back to the second state

            } else {
                curr_task->header_term_token_status = 0; // start over

            }

        } else {
            printf("Should not get here!");
            assert(false);
        }

        // put byte into parser header
        parse_buf[curr_task->parse_buf_idx] = data;
        curr_task->parse_buf_idx++;
        if(curr_task->parse_buf_idx == BUF_DATA_MAXSIZE){ // parser buffer overflow
            if(curr_task->header_term_token_status != HEADER_TERM_STATUS)
                return PARSER_BUF_OVERFLOW;
        }
    } // end of while
    assert(curr_task->header_term_token_status == HEADER_TERM_STATUS);
    return EXIT_SUCCESS;
}

void print_req(Request* request)
{
    int index;
    printf("Http Method: %s\n",request->http_method);
    printf("Http Version: %s\n",request->http_version);
    printf("Http Uri: %s\n",request->http_uri);
    for(index = 0;index < request->header_count;index++){
        printf("Request Header\n");
        printf("Header name: %s\nHeader Value: %s\n\n",request->headers[index].header_name,request->headers[index].header_value);
    }
}