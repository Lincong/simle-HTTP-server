//
// Created by Lincong Li on 1/12/18.
//

#include "http.h"
#include "message.h"
#include "logger.h"
#include "parser/parse.h"

int read_header_data(peer_t *peer);
void print_parse_buf(http_task_t* curr_task);
void print_req(Request* request);
bool valid_response_code(int response_code);

// there are 3 parts of body: status line, header, and body
int generate_response_status_line(http_task_t* http_task, char *code, char*reason);
int generate_response_header(http_task_t* http_task, char *hname, char *hvalue);
int generate_nonbody_response(http_task_t* http_task, int response_code);
int generate_response_msg(http_task_t* http_task, char *msg);

int send_nonbody_reponse(peer_t *peer);

void send_404(http_task_t* http_task);

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
    int response_code = -1;

    // RECV_HEADER_STATE -> RECV_BODY_STATE -> GENERATE_HEADER_STATE ->
    // SEND_HEADER_STATE -> SEND_BODY_STATE -> FINISHED_STATE
    while(curr_task->state != FINISHED_STATE){
        if(curr_task->state == RECV_HEADER_STATE){
            // read bytes from receiving buffer into parser buffer
            COMM_LOG("%s", "In RECV_HEADER_STATE")
            int ret = read_header_data(peer);
            if(ret == PARSER_BUF_OVERFLOW) {
                return CLOSE_CONN_IMMEDIATELY; // invalid header request

            } else if(ret == EMPTY_RECV_BUF) {
                return KEEP_CONN;

            }
            HTTP_LOG("%s", "Data for parser is ready")
            HTTP_LOG("parse_buf_idx: %d", (int)curr_task->parse_buf_idx)
            if(HTTP_LOG_ON)
                print_parse_buf(curr_task);

            // let parser take care of received data
            Request *request = parse((char*)curr_task->parse_buf, (int)curr_task->parse_buf_idx);
            if(request == NULL) {
                HTTP_LOG("%s", "parse() returns NULL")
                exit(0);
                return CLOSE_CONN_IMMEDIATELY; // invalid header request
            }
            if(HTTP_LOG_ON)
                print_req(request);

            // handle request
            char * http_method = request->http_method;
            if (!strcmp(http_method, "HEAD")) {
                response_code = NOT_IMPLEMENTED_NUM;

            } else if (!strcmp(http_method, "GET")) {

            } else if (!strcmp(http_method, "POST")) {
                response_code = NOT_IMPLEMENTED_NUM;

            } else { // not implemented
                response_code = NOT_IMPLEMENTED_NUM;
            }

            // 404
            response_code = BAD_REQUEST_NUM;

            curr_task->state = GENERATE_HEADER_STATE;

            free(request->headers);
            free(request);

        }else if(curr_task->state == RECV_BODY_STATE){
            COMM_LOG("%s", "In RECV_BODY_STATE")

            curr_task->state = GENERATE_HEADER_STATE;

        }else if(curr_task->state == GENERATE_HEADER_STATE){
            COMM_LOG("%s", "In GENERATE_HEADER_STATE")
            // after generate_response_header, either stays in the current state or go to send header state
            curr_task->state = generate_nonbody_response(curr_task, response_code);

        }else if(curr_task->state == SEND_HEADER_STATE){
            COMM_LOG("%s", "In SEND_HEADER_STATE")
            // copy data from the response buffer to sending buffer of the current connection
            int ret = send_nonbody_reponse(peer);
            if(ret == SEND_BODY_STATE) // stay in the current state
                return KEEP_CONN;

            // go to next state basing on the response code
            if(response_code == BAD_REQUEST_NUM ||
               response_code == NOT_FOUND_NUM ||
               response_code == LENGTH_REQUIRE_NUM ||
               response_code == INTERNAL_SERVER_ERROR_NUM ||
               response_code == NOT_IMPLEMENTED_NUM ||
               response_code == HTTP_VERSION_NOT_SUPPORTED_NUM) {
                curr_task->state = FINISHED_STATE;
            }


            // curr_task->state = SEND_BODY_STATE;

        }else if(curr_task->state == SEND_BODY_STATE){
            COMM_LOG("%s", "In SEND_BODY_STATE")



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
    curr_task->header_term_token_status = 0; // reset state to the initial state
    return EXIT_SUCCESS;
}


void print_parse_buf(http_task_t* curr_task)
{
    printf("Parse buffer:\n");
    int i;
    for(i = 0; i < curr_task->parse_buf_idx; i++)
        printf("%c", curr_task->parse_buf[i]);
    printf("Parse buffer ends\n");
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

bool valid_response_code(int response_code)
{
    if(HTTP_LOG_ON)
        printf("Response code is %d\n", response_code);
    return (response_code == 404 ||
            response_code == 400 ||
            response_code == 411 ||
            response_code == 500 ||
            response_code == 501 ||
            response_code == 505);
}

// generate response headers basing on the response code
int generate_nonbody_response(http_task_t* http_task, int response_code)
{
    assert(valid_response_code(response_code));
    switch (response_code){
        case BAD_REQUEST_NUM:
            generate_response_status_line(http_task, CODE_400, BAD_REQUEST);
            generate_response_header(http_task, CONNECTION, CLOSE);
            generate_response_msg(http_task, CLRF);
            return SEND_HEADER_STATE;

        case NOT_FOUND_NUM:
            generate_response_status_line(http_task, CODE_404, NOT_FOUND);
            generate_response_msg(http_task, CLRF);
            return SEND_HEADER_STATE;

        case LENGTH_REQUIRE_NUM:
            generate_response_status_line(http_task, CODE_411, LENGTH_REQUIRE);
            generate_response_header(http_task, CONNECTION, CLOSE);
            generate_response_msg(http_task, CLRF);
            return SEND_HEADER_STATE;

        case INTERNAL_SERVER_ERROR_NUM:
            generate_response_status_line(http_task, CODE_500, INTERNAL_SERVER_ERROR);
            generate_response_header(http_task, CONNECTION, CLOSE);
            generate_response_msg(http_task, CLRF);
            return SEND_HEADER_STATE;

        case NOT_IMPLEMENTED_NUM:
            generate_response_status_line(http_task, CODE_501, NOT_IMPLEMENTED);
            generate_response_header(http_task, CONNECTION, CLOSE);
            generate_response_msg(http_task, CLRF);
            return SEND_HEADER_STATE;

        case HTTP_VERSION_NOT_SUPPORTED_NUM:
            generate_response_status_line(http_task, CODE_505, HTTP_VERSION_NOT_SUPPORTED);
            generate_response_header(http_task, CONNECTION, CLOSE);
            generate_response_msg(http_task, CLRF);
            return SEND_HEADER_STATE;

        default:
            break;
    }
    return http_task->state;
}

// assume this function will never fail for now
int generate_response_status_line(http_task_t* http_task, char *code, char*reason)
{
    int ret;
    ret = buf_write(&http_task->response_buf, HTTP_VERSION, strlen(HTTP_VERSION));
    ret = buf_write(&http_task->response_buf, SP, strlen(SP));
    ret = buf_write(&http_task->response_buf, code, strlen(code));
    ret = buf_write(&http_task->response_buf, SP, strlen(SP));
    ret = buf_write(&http_task->response_buf, reason, strlen(reason));
    ret = buf_write(&http_task->response_buf, CLRF, strlen(CLRF));
    assert(ret >= 0);
    return EXIT_SUCCESS;
}

// assume this function will never fail for now
int generate_response_header(http_task_t* http_task, char *hname, char *hvalue)
{
    int ret;
    ret = buf_write(&http_task->response_buf, hname, strlen(hname));
    ret = buf_write(&http_task->response_buf, COLON, strlen(COLON));
    ret = buf_write(&http_task->response_buf, SP, strlen(SP));
    ret = buf_write(&http_task->response_buf, hvalue, strlen(hvalue));
    // two CLRF indicates the end of the response header section
    ret = buf_write(&http_task->response_buf, CLRF, strlen(CLRF));
    assert(ret >= 0);
    return EXIT_SUCCESS;
}

int generate_response_msg(http_task_t* http_task, char *msg)
{
    assert(buf_write(&http_task->response_buf, msg, strlen(msg)) > 0);
    return EXIT_SUCCESS;
}

int send_nonbody_reponse(peer_t *peer)
{
    cbuf_t* send_buf = &peer->sending_buffer;
    cbuf_t* response_buf = &peer->http_task->response_buf;
    uint8_t data;
    while(!buf_empty(response_buf)){
        // read one byte from the response buffer
        buf_get(response_buf, &data);

        if(!buf_full(send_buf)){
            buf_put(send_buf, data);
        } else { // sending buffer is full so stay in the current state
            return SEND_HEADER_STATE;
        }
    }
    return EXIT_SUCCESS;
}
// write 404 header to the response buffer of that client
void send_404(http_task_t* http_task)
{
//    send_response(client_buffer, (char*)"400", (char*)"Bad Request");
//    send_header(client_buffer, "Connection", "Close");
//    send_msg(client_buffer, clrf);
}
