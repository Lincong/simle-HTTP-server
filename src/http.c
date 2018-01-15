//
// Created by Lincong Li on 1/12/18.
//

#include "http.h"
#include "message.h"
#include "logger.h"
#include "parser/parse.h"
#include "file_handlers.h"

extern char *WWW_DIR;

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
void get_mime_type(char *mime, char *type);
char* get_header_value(Request *request, char * hname);
int generate_GET_header(http_task_t* http_task, Request* request, bool last_req);
int stream_file_content(peer_t* peer);

void chopN(char *str, size_t n);
bool starts_with(const char *pre, const char *str);
void remove_redundancy_from_uri(char* uri);

/*
  _   _ _____ _____ ____    ____                            _     _                     _ _
 | | | |_   _|_   _|  _ \  |  _ \ ___  __ _ _   _  ___  ___| |_  | |__   __ _ _ __   __| | | ___  ___
 | |_| | | |   | | | |_) | | |_) / _ \/ _` | | | |/ _ \/ __| __| | '_ \ / _` | '_ \ / _` | |/ _ \/ __|
 |  _  | | |   | | |  __/  |  _ <  __/ (_| | |_| |  __/\__ \ |_  | | | | (_| | | | | (_| | |  __/\__ \
 |_| |_| |_|   |_| |_|     |_| \_\___|\__, |\__,_|\___||___/\__| |_| |_|\__,_|_| |_|\__,_|_|\___||___/
                                         |_|
*/


// This function should not have any state
int handle_http(peer_t *peer)
{
    HTTP_LOG("%s", "In HTTP handler!")
    http_task_t* curr_task = peer->http_task;
    if(curr_task == NULL){
        fprintf(stderr, "When handle_http() is called, http_task should have been allocated");
        exit(EXIT_FAILURE);
    }

    while(curr_task->state != FINISHED_STATE){
        if(curr_task->state == RECV_HEADER_STATE){
            // read bytes from receiving buffer into parser buffer
            COMM_LOG("%s", "In RECV_HEADER_STATE")
            assert(curr_task->response_code == -1);
            assert(curr_task->last_request == false);
            assert(curr_task->method_type == NO_METHOD);
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
                return CLOSE_CONN_IMMEDIATELY; // invalid header request
            }
            if(HTTP_LOG_ON)
                print_req(request);

            // check if the current request is the last one
            if (!strcmp(get_header_value(request, "Connection"), "Close")) {
                HTTP_LOG("%s", "Last request connections");
                curr_task->last_request = true;
            }


            // handle request
            char * http_method = request->http_method;
            if (!strcmp(http_method, "HEAD")) {
                HTTP_LOG("%s", "HEAD method!")
                curr_task->method_type = HEAD_METHOD;

            } else if (!strcmp(http_method, "GET")) {
                HTTP_LOG("%s", "GET method!")
                curr_task->method_type = GET_METHOD;

            } else if (!strcmp(http_method, "POST")) {
                HTTP_LOG("%s", "POST method!")
                curr_task->method_type = POST_METHOD;

            } else {
                curr_task->response_code = NOT_IMPLEMENTED_NUM;
                curr_task->state = GENERATE_HEADER_STATE;
                free(request->headers);
                free(request);
                continue;
            }


            if(curr_task->method_type == GET_METHOD || curr_task->method_type == HEAD_METHOD) {
                curr_task->response_code = generate_GET_header(curr_task, request, curr_task->last_request);
                if(curr_task->response_code == OK_NUM) {
                    curr_task->state = SEND_HEADER_STATE;

                } else {
                    curr_task->state = GENERATE_HEADER_STATE;

                }

            } else { // POST_METHOD
                curr_task->state = RECV_BODY_STATE;

            }
            free(request->headers);
            free(request);

        } else if(curr_task->state == RECV_BODY_STATE) {
            COMM_LOG("%s", "In RECV_BODY_STATE")
            assert(curr_task->method_type == POST_METHOD); // only POST method has a body
            // TODO: handle POST body

            curr_task->response_code = OK_NUM;
            curr_task->state = GENERATE_HEADER_STATE;

        } else if(curr_task->state == GENERATE_HEADER_STATE) {
            COMM_LOG("%s", "In GENERATE_HEADER_STATE")
            // after generate_response_header, either stays in the current state or go to send header state
            curr_task->state = generate_nonbody_response(curr_task, curr_task->response_code);

        } else if(curr_task->state == SEND_HEADER_STATE) {
            COMM_LOG("%s", "In SEND_HEADER_STATE")
            // copy data from the response buffer to sending buffer of the current connection
            int ret = send_nonbody_reponse(peer);
            if(ret == SEND_BODY_STATE) // stay in the current state
                return KEEP_CONN;

            // go to next state basing on the response code
            if(curr_task->response_code == BAD_REQUEST_NUM ||
                    curr_task->response_code == NOT_FOUND_NUM ||
                    curr_task->response_code == LENGTH_REQUIRE_NUM ||
                    curr_task->response_code == INTERNAL_SERVER_ERROR_NUM ||
                    curr_task->response_code == NOT_IMPLEMENTED_NUM ||
                    curr_task->response_code == HTTP_VERSION_NOT_SUPPORTED_NUM) {

                curr_task->state = FINISHED_STATE;

            } else { // take care of normal states

                if(curr_task->method_type == GET_METHOD) {
                    curr_task->state = SEND_BODY_STATE;

                }else if(curr_task->method_type == HEAD_METHOD) {
                    curr_task->state = FINISHED_STATE; // no body to send

                } else if (curr_task->method_type == POST_METHOD) {
                    curr_task->state = FINISHED_STATE; // no body to send

                } else {
                    HTTP_LOG("%s", "Shouldn't get here");
                    assert(false);
                }
            }

        } else if(curr_task->state == SEND_BODY_STATE) {
            COMM_LOG("%s", "In SEND_BODY_STATE")

            assert(curr_task->method_type == GET_METHOD); // for now only GET gets here
            int ret = stream_file_content(peer);
            if(ret == STREAM_FILE_NOT_DONE) // if file content has not been all sent out
                return KEEP_CONN;

            else if(ret == STREAM_FILE_ERROR)
                return CLOSE_CONN_IMMEDIATELY;

            else if(ret == STREAM_FILE_DONE)
                curr_task->state = FINISHED_STATE;

            else
                assert(false); // shouldn't get here

        } else {
            fprintf(stderr, "Shouldn't get to this unknown state");
            exit(EXIT_FAILURE);

        }
    }
    reset_http_task(curr_task);
    return (curr_task->last_request ? CLOSE_CONN : KEEP_CONN);
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
    // char* terminator = "\r\n\r\n";
    uint8_t data;
    while(curr_task->header_term_token_status != HEADER_TERM_STATUS){
        if(buf_empty(&peer->receiving_buffer)) // no more to read from the receiving buffer
            return EMPTY_RECV_BUF;
        buf_get(&peer->receiving_buffer, &data);

        if(curr_task->header_term_token_status == 0){
            if(data == CL)
                curr_task->header_term_token_status = 1;
            // otherwise, state is still 0

        } else if(curr_task->header_term_token_status == 1) {
            if(data == RF) { // second character is '\n'
                curr_task->header_term_token_status = 2; // move to next state

            } else if(data != CL){
               curr_task->header_term_token_status = 0; // start over
            }

        } else if(curr_task->header_term_token_status == 2) {
            if(data == CL) { // second character is '\r'
                curr_task->header_term_token_status = 3; // move to next state

            } else if(data == CL) {
                curr_task->header_term_token_status = 1; // go back to the second state

            } else {
                curr_task->header_term_token_status = 0; // start over

            }

        } else if(curr_task->header_term_token_status == 3) {
            if(data == RF) { // second character is '\n'
                curr_task->header_term_token_status = HEADER_TERM_STATUS; // move to next state

            } else if(data == CL) {
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
    assert(ret >= 0);
    ret = buf_write(&http_task->response_buf, COLON, strlen(COLON));
    assert(ret >= 0);
    ret = buf_write(&http_task->response_buf, SP, strlen(SP));
    assert(ret >= 0);
    ret = buf_write(&http_task->response_buf, hvalue, strlen(hvalue));
    assert(ret >= 0);
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

// Get accordingly MIME type given file extension name
void get_mime_type(char *mime, char *type)
{
    if (!strcmp(mime, "html")) {
        strcpy(type, TEXT_HTML);
    } else if (!strcmp(mime, CSS)) {
        strcpy(type, TEXT_CSS);
    } else if (!strcmp(mime, PNG)) {
        strcpy(type, IMAGE_PNG);
    } else if (!strcmp(mime, JPEG)) {
        strcpy(type, IMAGE_JPEG);
    } else if (!strcmp(mime, GIF)) {
        strcpy(type, IMAGE_GIF);
    } else {
        strcpy(type, APPLICATION_OCTET_STREAM);
    }
}

// Get value given header name.
// If header name not found, hvalue param will not be set.
char* get_header_value(Request *request, char * hname) {
    int i;

    for (i = 0; i < request->header_count; i++) {
        if (!strcmp(request->headers[i].header_name, hname)) {
            return request->headers[i].header_value;
        }
    }
    return "";
}

void chopN(char *str, size_t n)
{
    assert(n != 0 && str != 0);
    size_t len = strlen(str);
    if (n > len)
        return;  // Or: n = len;
    memmove(str, str+n, len - n + 1);
}

void remove_redundancy_from_uri(char* uri)
{
    char* redundant_str = "http://";
    if(starts_with(redundant_str, uri)){
        chopN(uri, strlen(redundant_str));
    }
    if(uri[strlen(uri) - 1] == '/')
        uri[strlen(uri) - 1] = 0;
}

bool starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

int generate_GET_header(http_task_t* http_task, Request* request, bool last_req)
{
    HTTP_LOG("%s", "In generate_GET_header()");
    char fullpath[4096];
    char extension[64];
    char mime_type[64];
    char curr_time[256];
    char last_modified[256];
    size_t content_length;
    char content_len_str[16];

    strcpy(fullpath, WWW_DIR);

    remove_redundancy_from_uri(request->http_uri);
    strcat(fullpath, request->http_uri);

// for testing
    if (requested_path_is_dir(fullpath)) // end with "/"
        strcat(fullpath, INDEX_FILE);

    if (access(fullpath, F_OK) < 0) {
        HTTP_LOG("Path %s can not be accessed", fullpath)
        return NOT_FOUND_NUM;
    }

    // first generate header information and write it to the response buffer
    // get file meta data
    // Get content type
    get_extension(fullpath, extension);
    str_tolower(extension);
    get_mime_type(extension, mime_type);

    /* Get content length */
    content_length = get_file_len(fullpath);
    sprintf(content_len_str, "%zu", content_length);

    /* Get current time */
    get_curr_time(curr_time, 256);

    /* Get last modified time */
    get_flmodified(fullpath, last_modified, 256);

    generate_response_status_line(http_task, CODE_200, OK);
    generate_response_header(http_task, "Server", SERVER_NAME);
    generate_response_header(http_task, "Date", curr_time);
    generate_response_header(http_task, "Content-Length", content_len_str);
    generate_response_header(http_task, "Content-Type", mime_type);
    generate_response_header(http_task, "Last-modified", last_modified);

    if(last_req)
        generate_response_header(http_task, CONNECTION, CLOSE);
    else
        generate_response_header(http_task, CONNECTION, KEEP_ALIVE);

    generate_response_msg(http_task, CLRF);

    // open the target file to read and store its fd to the current http_task
    FILE* fd = fopen(fullpath, "r");
    if (fd == NULL) {
        HTTP_LOG("Error when opening file %s for reading", fullpath);
        return INTERNAL_SERVER_ERROR_NUM;
    }
    http_task->fp = fd;
    // move on to send file content
    HTTP_LOG("%s", "End of generate_GET_header");
    return OK_NUM;
}

int stream_file_content(peer_t* peer)
{
    if(peer == NULL) return STREAM_FILE_ERROR;
    if(peer->http_task == NULL) return STREAM_FILE_ERROR;

    cbuf_t* sending_buf = &peer->sending_buffer;
    char data;
    while(!buf_full(sending_buf)) {
        data = (char)fgetc(peer->http_task->fp);
        if(ferror(peer->http_task->fp)){
            clearerr(peer->http_task->fp);
            fclose(peer->http_task->fp);
            HTTP_LOG("%s", "fgetc() returns error")
            return STREAM_FILE_ERROR;
        }

        buf_put(sending_buf, (uint8_t)data);
        if(feof(peer->http_task->fp)){
            fclose(peer->http_task->fp);
            HTTP_LOG("%s", "fget() returns eof")
            return STREAM_FILE_DONE;
        }
    }
    HTTP_LOG("%s", "Buffer is full")
    return STREAM_FILE_NOT_DONE;
}