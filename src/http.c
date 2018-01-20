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

// HTTP
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
void free_request(Request* req);

// CGI

//CGI_param *init_CGI_param();

void build_CGI_param(CGI_param*, Request*, host_and_port);
void print_CGI_param(CGI_param*);

char *new_string(char *);
void set_envp_field_by_str (char *, char *, char *, CGI_param*, int);
void set_envp_field_with_header (Request *, char *, char *, char *, CGI_param*, int);

//void print_executor(CGI_executor *);
void* get_in_addr(sockaddr *sa);

void execve_error_handler();
void start_CGI_script(peer_t* client, CGI_param *cgi_parameter, char *post_body, size_t content_length);

/*
  _   _ _____ _____ ____    ____                            _     _                     _ _
 | | | |_   _|_   _|  _ \  |  _ \ ___  __ _ _   _  ___  ___| |_  | |__   __ _ _ __   __| | | ___  ___
 | |_| | | |   | | | |_) | | |_) / _ \/ _` | | | |/ _ \/ __| __| | '_ \ / _` | '_ \ / _` | |/ _ \/ __|
 |  _  | | |   | | |  __/  |  _ <  __/ (_| | |_| |  __/\__ \ |_  | | | | (_| | | | | (_| | |  __/\__ \
 |_| |_| |_|   |_| |_|     |_| \_\___|\__, |\__,_|\___||___/\__| |_| |_|\__,_|_| |_|\__,_|_|\___||___/
                                         |_|
*/

Request *request;

// This function should not have any state
int handle_http(peer_t *peer) {
    HTTP_LOG("%s", "In HTTP handler!")
    http_task_t *curr_task = peer->http_task;
    if (curr_task == NULL) {
        fprintf(stderr, "When handle_http() is called, http_task should have been allocated");
        exit(EXIT_FAILURE);
    }

    while (curr_task->state != FINISHED_STATE) {
        if (curr_task->state == RECV_HEADER_STATE) {
            // read bytes from receiving buffer into parser buffer
            COMM_LOG("%s", "In RECV_HEADER_STATE")
            assert(curr_task->response_code == -1);
            assert(curr_task->last_request == false);
            assert(curr_task->method_type == NO_METHOD);
            int ret = read_header_data(peer);
            if (ret == PARSER_BUF_OVERFLOW) {
                return CLOSE_CONN_IMMEDIATELY; // invalid header request

            } else if (ret == EMPTY_RECV_BUF) {
                return KEEP_CONN;

            }
            HTTP_LOG("%s", "Data for parser is ready")
            HTTP_LOG("parse_buf_idx: %d", (int) curr_task->parse_buf_idx)
            if (HTTP_LOG_ON)
                print_parse_buf(curr_task);

            // let parser take care of received data
            request = parse((char *) curr_task->parse_buf, (int) curr_task->parse_buf_idx);
            if (request == NULL) {
                HTTP_LOG("%s", "parse() returns NULL")
                return CLOSE_CONN_IMMEDIATELY; // invalid header request
            }
            if (HTTP_LOG_ON)
                print_req(request);

            // check if the current request is the last one
            if (!strcmp(get_header_value(request, "Connection"), "Close")) {
                HTTP_LOG("%s", "Last request connections");
                curr_task->last_request = true;
            }

            // handle request
            // check what method does the request contains
            char *http_method = request->http_method;
            if (!strcmp(http_method, "HEAD")) {
                HTTP_LOG("%s", "HEAD method!")
                curr_task->method_type = HEAD_METHOD;
                curr_task->state = CHECK_CGI;

            } else if (!strcmp(http_method, "GET")) {
                HTTP_LOG("%s", "GET method!")
                curr_task->method_type = GET_METHOD;
                curr_task->state = CHECK_CGI;

            } else if (!strcmp(http_method, "POST")) {
                HTTP_LOG("%s", "POST method!")
                curr_task->method_type = POST_METHOD;
                // check Content-Length for POST
                char *content_length_str;
                content_length_str = get_header_value(request, "Content-Length");
                if (strlen(content_length_str) == 0) {
                    // go to generate corresponding error header
                    curr_task->response_code = LENGTH_REQUIRE_NUM;
                    curr_task->state = GENERATE_HEADER_STATE;

                } else {
                    int content_len = atoi(content_length_str);
                    HTTP_LOG("POST content length: %d", content_len)
                    assert(curr_task->body_bytes_num == 0);
                    curr_task->body_bytes_num = (size_t) content_len;
                    curr_task->post_body = (char*) malloc(curr_task->body_bytes_num * sizeof(char));
                    curr_task->post_body_idx = 0;
                    curr_task->state = RECV_BODY_STATE; // go to RECV_BODY_STATE
                }

            } else {
                curr_task->response_code = NOT_IMPLEMENTED_NUM;
                curr_task->state = GENERATE_HEADER_STATE;
                free_request(request); // otherwise don't free the request yet
            }

        } else if (curr_task->state == RECV_BODY_STATE) {
            COMM_LOG("%s", "In RECV_BODY_STATE")
            // just read out all bytes that belong to the message body from the receiving buffer
            cbuf_t *receiving_buffer = &peer->receiving_buffer;
            uint8_t data;
            while (curr_task->post_body_idx < curr_task->body_bytes_num) {
                if (buf_empty(receiving_buffer)) // read from the receiving buffer later
                    return KEEP_CONN;
                buf_get(receiving_buffer, &data);
                curr_task->post_body[curr_task->post_body_idx] = data;
                curr_task->post_body_idx++;
            }
            HTTP_LOG("%s", "Finished receiving body")
            curr_task->state = CHECK_CGI; // after receiving POST body, go check if it is a CGI request

        } else if (curr_task->state == GENERATE_HEADER_STATE) {
            COMM_LOG("%s", "In GENERATE_HEADER_STATE")
            // after generate_response_header, either stays in the current state or go to send header state
            curr_task->state = generate_nonbody_response(curr_task, curr_task->response_code);

        } else if (curr_task->state == SEND_HEADER_STATE) {
            COMM_LOG("%s", "In SEND_HEADER_STATE")
            // copy data from the response buffer to sending buffer of the current connection
            int ret = send_nonbody_reponse(peer);
            if (ret == SEND_BODY_STATE) // stay in the current state
                return KEEP_CONN;

            // go to next state basing on the response code
            if (curr_task->response_code == BAD_REQUEST_NUM ||
                curr_task->response_code == NOT_FOUND_NUM ||
                curr_task->response_code == LENGTH_REQUIRE_NUM ||
                curr_task->response_code == INTERNAL_SERVER_ERROR_NUM ||
                curr_task->response_code == NOT_IMPLEMENTED_NUM ||
                curr_task->response_code == HTTP_VERSION_NOT_SUPPORTED_NUM) {

                curr_task->state = FINISHED_STATE;

            } else { // take care of normal states

                if (curr_task->method_type == GET_METHOD) {
                    curr_task->state = SEND_BODY_STATE;

                } else if (curr_task->method_type == HEAD_METHOD) {
                    curr_task->state = FINISHED_STATE; // no body to send for HEAD

                } else if (curr_task->method_type == POST_METHOD) {
                    curr_task->state = FINISHED_STATE; // no body to send for POST

                } else {
                    HTTP_LOG("%s", "Shouldn't get here");
                    assert(false);
                }
            }

        } else if (curr_task->state == CHECK_CGI) {

            // TODO
            char *cgi_prefix = "/cgi/";
            char prefix[8];
            memset(prefix, 0, 8);
            if (strlen(request->http_uri) >= strlen(cgi_prefix))
                snprintf(prefix, strlen(cgi_prefix) + 1, "%s", request->http_uri);

            CGI_LOG("prefix: %s", prefix);
            if (!strcmp(cgi_prefix, prefix)) { // Handle dynamic http request
                CGI_LOG("%s", "It's a CGI request!")
                CGI_param *cgi_parameters = init_CGI_param();
                host_and_port has;
                char remoteIP[INET6_ADDRSTRLEN];
                has.host = (char *) inet_ntop(peer->addres.sin_family,
                                              get_in_addr((sockaddr *) &peer->addres),
                                              remoteIP,
                                              INET6_ADDRSTRLEN);
                has.port = peer->addres.sin_port;
                build_CGI_param(cgi_parameters, request, has);
                print_CGI_param(cgi_parameters);

                if (!strcmp(request->http_method, "POST")) {
                    if (curr_task->body_bytes_num > 0) {
                        start_CGI_script(peer, cgi_parameters, curr_task->post_body, (size_t)curr_task->body_bytes_num);
                        return CGI_READY_FOR_WRITE;
                    }
                }

                // handle CGI for GET and HEAD
                start_CGI_script(peer, cgi_parameters, NULL, 0);
                return CGI_READY_FOR_READ; // since args are sent to the child process via env variables, we can just read from it

            } else { // static content
                CGI_LOG("%s", "It's a static content request!")
                // do something depending on the method
                if(curr_task->method_type == GET_METHOD || curr_task->method_type == HEAD_METHOD) {
                    curr_task->response_code = generate_GET_header(curr_task, request, curr_task->last_request);
                    if (curr_task->response_code == OK_NUM) {
                        curr_task->state = SEND_HEADER_STATE; // the OK 200 header is already generated by generate_GET_header()

                    } else {
                        curr_task->state = GENERATE_HEADER_STATE; // generate non-OK 200 header

                    }
                } else {
                    curr_task->response_code = OK_NUM;
                    curr_task->state = GENERATE_HEADER_STATE;
                }
            }

        } else if (curr_task->state == SEND_BODY_STATE) {
            COMM_LOG("%s", "In SEND_BODY_STATE")
            assert(curr_task->method_type == GET_METHOD); // for now only GET gets here

            int ret = stream_file_content(peer);
            if (ret == STREAM_FILE_NOT_DONE) // if file content has not been all sent out
                return KEEP_CONN;

            else if (ret == STREAM_FILE_ERROR)
                return CLOSE_CONN_IMMEDIATELY;

            else if (ret == STREAM_FILE_DONE)
                curr_task->state = FINISHED_STATE;

            else
                assert(false); // shouldn't get here

        } else {
            fprintf(stderr, "Shouldn't get to this unknown state");
            exit(EXIT_FAILURE);

        }
    }
    free_request(request);
    reset_http_task(curr_task);
    return (curr_task->last_request ? CLOSE_CONN : KEEP_CONN);
}
// HTTP request handler helpers

// read bytes from receiving buffer of the client to parser buffer of it
// return EMPTY_RECV_BUF | EXIT_SUCCESS | PARSER_BUF_OVERFLOW
int read_header_data(peer_t *peer) {
    assert(peer != NULL);
    http_task_t *curr_task = peer->http_task;
    assert(curr_task != NULL);
    uint8_t *parse_buf = curr_task->parse_buf;
    // char* terminator = "\r\n\r\n";
    uint8_t data;
    while (curr_task->header_term_token_status != HEADER_TERM_STATUS) {
        if (buf_empty(&peer->receiving_buffer)) // no more to read from the receiving buffer
            return EMPTY_RECV_BUF;
        buf_get(&peer->receiving_buffer, &data);
        if (curr_task->header_term_token_status == 0) {
            if (data == CL)
                curr_task->header_term_token_status = 1;
            // otherwise, state is still 0

        } else if (curr_task->header_term_token_status == 1) {
            if (data == RF) { // second character is '\n'
                curr_task->header_term_token_status = 2; // move to next state

            } else if (data != CL) {
                curr_task->header_term_token_status = 0; // start over
            }

        } else if (curr_task->header_term_token_status == 2) {
            if (data == CL) { // second character is '\r'
                curr_task->header_term_token_status = 3; // move to next state

            } else if (data == CL) {
                curr_task->header_term_token_status = 1; // go back to the second state

            } else {
                curr_task->header_term_token_status = 0; // start over

            }

        } else if (curr_task->header_term_token_status == 3) {
            if (data == RF) { // second character is '\n'
                curr_task->header_term_token_status = HEADER_TERM_STATUS; // move to next state

            } else if (data == CL) {
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
        if (curr_task->parse_buf_idx == BUF_DATA_MAXSIZE) { // parser buffer overflow
            if (curr_task->header_term_token_status != HEADER_TERM_STATUS)
                return PARSER_BUF_OVERFLOW;
        }
    } // end of while
    assert(curr_task->header_term_token_status == HEADER_TERM_STATUS);
    curr_task->header_term_token_status = 0; // reset state to the initial state
    return EXIT_SUCCESS;
}

void print_parse_buf(http_task_t *curr_task) {
        printf("Parse buffer:\n");
        int i;
        for (i = 0; i < curr_task->parse_buf_idx; i++)
            printf("%c", curr_task->parse_buf[i]);
        printf("Parse buffer ends\n");
    }

void print_req(Request *request) {
        int index;
        printf("Http Method: %s\n", request->http_method);
        printf("Http Version: %s\n", request->http_version);
        printf("Http Uri: %s\n", request->http_uri);
        for (index = 0; index < request->header_count; index++) {
            printf("Request Header\n");
            printf("Header name: %s\nHeader Value: %s\n\n", request->headers[index].header_name,
                   request->headers[index].header_value);
        }
    }

bool valid_response_code(int response_code) {
    if (HTTP_LOG_ON)
        printf("Response code is %d\n", response_code);
    return (response_code == OK_NUM ||
            response_code == NOT_FOUND_NUM ||
            response_code == BAD_REQUEST_NUM ||
            response_code == LENGTH_REQUIRE_NUM ||
            response_code == INTERNAL_SERVER_ERROR_NUM ||
            response_code == NOT_IMPLEMENTED_NUM ||
            response_code == HTTP_VERSION_NOT_SUPPORTED_NUM);
}

// generate response headers basing on the response code
int generate_nonbody_response(http_task_t *http_task, int response_code) {
    HTTP_LOG("%s", "In generate_nonbody_response()")
    assert(valid_response_code(response_code));
    switch (response_code) {
        case OK_NUM:
            HTTP_LOG("%s", "Generating 200")
            generate_response_status_line(http_task, CODE_200, OK);
            generate_response_header(http_task, "Server", SERVER_NAME);
            return SEND_HEADER_STATE;

        case BAD_REQUEST_NUM:
            HTTP_LOG("%s", "Generating 400")
            generate_response_status_line(http_task, CODE_400, BAD_REQUEST);
            generate_response_header(http_task, CONNECTION, CLOSE);
            generate_response_msg(http_task, CLRF);
            return SEND_HEADER_STATE;

        case NOT_FOUND_NUM:
            HTTP_LOG("%s", "Generating 404")
            generate_response_status_line(http_task, CODE_404, NOT_FOUND);
            generate_response_msg(http_task, CLRF);
            return SEND_HEADER_STATE;

        case LENGTH_REQUIRE_NUM:
            HTTP_LOG("%s", "Generating 411")
            generate_response_status_line(http_task, CODE_411, LENGTH_REQUIRE);
            generate_response_header(http_task, CONNECTION, CLOSE);
            generate_response_msg(http_task, CLRF);
            return SEND_HEADER_STATE;

        case INTERNAL_SERVER_ERROR_NUM:
            HTTP_LOG("%s", "Generating 500")
            generate_response_status_line(http_task, CODE_500, INTERNAL_SERVER_ERROR);
            generate_response_header(http_task, CONNECTION, CLOSE);
            generate_response_msg(http_task, CLRF);
            return SEND_HEADER_STATE;

        case NOT_IMPLEMENTED_NUM:
            HTTP_LOG("%s", "Generating 501")
            generate_response_status_line(http_task, CODE_501, NOT_IMPLEMENTED);
            generate_response_header(http_task, CONNECTION, CLOSE);
            generate_response_msg(http_task, CLRF);
            return SEND_HEADER_STATE;

        case HTTP_VERSION_NOT_SUPPORTED_NUM:
            HTTP_LOG("%s", "Generating 505")
            generate_response_status_line(http_task, CODE_505, HTTP_VERSION_NOT_SUPPORTED);
            generate_response_header(http_task, CONNECTION, CLOSE);
            generate_response_msg(http_task, CLRF);
            return SEND_HEADER_STATE;

        default:
            break;
    }
    HTTP_LOG("%s", "Not generating anything")
    return http_task->state;
}

// assume this function will never fail for now
int generate_response_status_line(http_task_t *http_task, char *code, char *reason) {
    int ret;
    ret = buf_write(&http_task->response_buf, (uint8_t*) HTTP_VERSION, strlen(HTTP_VERSION));
    ret = buf_write(&http_task->response_buf, (uint8_t*) SP, strlen(SP));
    ret = buf_write(&http_task->response_buf, (uint8_t*) code, strlen(code));
    ret = buf_write(&http_task->response_buf, (uint8_t*) SP, strlen(SP));
    ret = buf_write(&http_task->response_buf, (uint8_t*) reason, strlen(reason));
    ret = buf_write(&http_task->response_buf, (uint8_t*) CLRF, strlen(CLRF));
    assert(ret >= 0);
    return EXIT_SUCCESS;
}

// assume this function will never fail for now
int generate_response_header(http_task_t *http_task, char *hname, char *hvalue) {
    int ret;
    ret = buf_write(&http_task->response_buf, (uint8_t*) hname, strlen(hname));
    assert(ret >= 0);
    ret = buf_write(&http_task->response_buf, (uint8_t*) COLON, strlen(COLON));
    assert(ret >= 0);
    ret = buf_write(&http_task->response_buf, (uint8_t*) SP, strlen(SP));
    assert(ret >= 0);
    ret = buf_write(&http_task->response_buf, (uint8_t*) hvalue, strlen(hvalue));
    assert(ret >= 0);
    // two CLRF indicates the end of the response header section
    ret = buf_write(&http_task->response_buf, (uint8_t*) CLRF, strlen(CLRF));
    assert(ret >= 0);
    return EXIT_SUCCESS;
}

int generate_response_msg(http_task_t *http_task, char *msg) {
    assert(buf_write(&http_task->response_buf, (uint8_t *) msg, strlen(msg)) > 0);
    return EXIT_SUCCESS;
}

int send_nonbody_reponse(peer_t *peer) {
    cbuf_t *send_buf = &peer->sending_buffer;
    cbuf_t *response_buf = &peer->http_task->response_buf;
    uint8_t data;
    while (!buf_empty(response_buf)) {
        // read one byte from the response buffer
        buf_get(response_buf, &data);

        if (!buf_full(send_buf)) {
            buf_put(send_buf, data);
        } else { // sending buffer is full so stay in the current state
            return SEND_HEADER_STATE;
        }
    }
    return EXIT_SUCCESS;
}

// Get accordingly MIME type given file extension name
void get_mime_type(char *mime, char *type) {
    HTTP_LOG("In get_mime_type(), mime is %s", mime)
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
char *get_header_value(Request *request, char *hname) {
    int i;

    for (i = 0; i < request->header_count; i++) {
        if (!strcmp(request->headers[i].header_name, hname)) {
            return request->headers[i].header_value;
        }
    }
    return "";
}

void chopN(char *str, size_t n) {
    assert(n != 0 && str != 0);
    size_t len = strlen(str);
    if (n > len)
        return;  // Or: n = len;
    memmove(str, str + n, len - n + 1);
}

void remove_redundancy_from_uri(char *uri) {
    char *redundant_str = "http://";
    if (starts_with(redundant_str, uri)) {
        chopN(uri, strlen(redundant_str));
    }
    if (uri[strlen(uri) - 1] == '/')
        uri[strlen(uri) - 1] = 0;
}

bool starts_with(const char *pre, const char *str) {
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

int generate_GET_header(http_task_t *http_task, Request *request, bool last_req) {
    HTTP_LOG("%s", "In generate_GET_header()");
    char fullpath[4096];
    const char *extension;
    char mime_type[64];
    char curr_time[256];
    char last_modified[256];
    size_t content_length;
    char content_len_str[16];

    strcpy(fullpath, WWW_DIR);

    HTTP_LOG("%s", "Before remove_redundancy_from_uri")
    remove_redundancy_from_uri(request->http_uri);
    HTTP_LOG("%s", "After remove_redundancy_from_uri")
    strcat(fullpath, request->http_uri);
    HTTP_LOG("%s", "After strcat()")
    // for testing
    if (requested_path_is_dir(fullpath)) { // end with "/"
        strcat(fullpath, INDEX_FILE);
        HTTP_LOG("%s", "1")
    }
    HTTP_LOG("%s", "2")
    HTTP_LOG("full requested path is: %s", fullpath)

    if (access(fullpath, F_OK) < 0) {
        HTTP_LOG("Path %s can not be accessed", fullpath)
        return NOT_FOUND_NUM;
    }

    // first generate header information and write it to the response buffer
    // get file meta data
    // Get content type
    HTTP_LOG("%s", "Before getting requested file information")
    HTTP_LOG("fullpath is %s", fullpath)
//    get_extension(fullpath, extension);
    extension = get_filename_ext(fullpath);
    str_tolower(extension);
    get_mime_type(extension, mime_type);

    /* Get content length */
    content_length = get_file_len(fullpath);
    sprintf(content_len_str, "%zu", content_length);

    /* Get current time */
    get_curr_time(curr_time, 256);

    /* Get last modified time */
    get_flmodified(fullpath, last_modified, 256);
    HTTP_LOG("%s", "After getting requested file information")

    HTTP_LOG("%s", "Before generating headers")
    generate_response_status_line(http_task, CODE_200, OK);
    generate_response_header(http_task, "Server", SERVER_NAME);
    generate_response_header(http_task, "Date", curr_time);
    generate_response_header(http_task, "Content-Length", content_len_str);
    generate_response_header(http_task, "Content-Type", mime_type);
    generate_response_header(http_task, "Last-modified", last_modified);
    HTTP_LOG("%s", "After generating headers")

    if (last_req)
        generate_response_header(http_task, CONNECTION, CLOSE);
    else
        generate_response_header(http_task, CONNECTION, KEEP_ALIVE);

    generate_response_msg(http_task, CLRF);

    // open the target file to read and store its fd to the current http_task
    FILE *fd = fopen(fullpath, "r");
    if (fd == NULL) {
        HTTP_LOG("Error when opening file %s for reading", fullpath);
        return INTERNAL_SERVER_ERROR_NUM;
    }
    http_task->fp = fd;
    // move on to send file content
    HTTP_LOG("%s", "End of generate_GET_header");
    return OK_NUM;
}

int stream_file_content(peer_t *peer) {
    if (peer == NULL) return STREAM_FILE_ERROR;
    if (peer->http_task == NULL) return STREAM_FILE_ERROR;

    cbuf_t *sending_buf = &peer->sending_buffer;
    char data;
    while (!buf_full(sending_buf)) {
        data = (char) fgetc(peer->http_task->fp);
        if (ferror(peer->http_task->fp)) {
            clearerr(peer->http_task->fp);
            fclose(peer->http_task->fp);
            HTTP_LOG("%s", "fgetc() returns error")
            return STREAM_FILE_ERROR;
        }

        buf_put(sending_buf, (uint8_t) data);
        if (feof(peer->http_task->fp)) {
            fclose(peer->http_task->fp);
            HTTP_LOG("%s", "fget() returns eof")
            return STREAM_FILE_DONE;
        }
    }
    HTTP_LOG("%s", "Buffer is full")
    return STREAM_FILE_NOT_DONE;
}

void free_request(Request *req) {
    free(req->headers);
    free(req);
}

// CGI

CGI_param *init_CGI_param() {
    CGI_param *ret = (CGI_param *) malloc(sizeof(CGI_param));
    if (ret == NULL) {
        CGI_LOG("%s", "[FATAL] Fails to allocate memory for CGI parametes")
        CGI_LOG("%s", "[FATAL] Fails to allocate memory for CGI parametes")
        assert(false);
    }
    return ret;
}

void build_CGI_param(CGI_param *param, Request *request, host_and_port hap) {
    param->filename = CGI_scripts;
    param->args[0] = CGI_scripts;
    param->args[1] = NULL;

    char buf[CGI_ENVP_INFO_MAXLEN];
    int index = 0;
    set_envp_field_by_str("GATEWAY_INTERFACE", "CGI/1.1", buf, param, index++);
    set_envp_field_by_str("SERVER_SOFTWARE", "Liso/1.0", buf, param, index++);
    set_envp_field_by_str("SERVER_PROTOCOL", "HTTP/1.1", buf, param, index++);
    set_envp_field_by_str("REQUEST_METHOD", request->http_method, buf, param, index++);
    set_envp_field_by_str("REQUEST_URI", request->http_uri, buf, param, index++);
    set_envp_field_by_str("SCRIPT_NAME", "/cgi", buf, param, index++);

    char path_info[512];
    memset(path_info, 0, 512);
    char query_string[512];
    memset(query_string, 0, 512);
    char *split_char;
    if ((split_char = strstr(request->http_uri, "?")) != NULL) {
        size_t split = split_char - request->http_uri;
        memcpy(query_string, request->http_uri + split + 1, strlen(request->http_uri) - split - 1);
        set_envp_field_by_str("QUERY_STRING", query_string, buf, param, index++);
        memcpy(path_info, request->http_uri + 4, split - 4);
        set_envp_field_by_str("PATH_INFO", path_info, buf, param, index++);
    } else {
        set_envp_field_by_str("QUERY_STRING", "", buf, param, index++);
        memcpy(path_info, request->http_uri + 4, strlen(request->http_uri) - 4);
        set_envp_field_by_str("PATH_INFO", path_info, buf, param, index++);
    }

    /* envp taken from request */
    set_envp_field_with_header(request, "Content-Length", "CONTENT_LENGTH", buf, param, index++);
    set_envp_field_with_header(request, "Content-Type", "CONTENT_TYPE", buf, param, index++);
    set_envp_field_with_header(request, "Accept", "HTTP_ACCEPT", buf, param, index++);
    set_envp_field_with_header(request, "Referer", "HTTP_REFERER", buf, param, index++);
    set_envp_field_with_header(request, "Accept-Encoding", "HTTP_ACCEPT_ENCODING", buf, param, index++);
    set_envp_field_with_header(request, "Accept-Language", "HTTP_ACCEPT_LANGUAGE", buf, param, index++);
    set_envp_field_with_header(request, "Accept-Charset", "HTTP_ACCEPT_CHARSET", buf, param, index++);
    set_envp_field_with_header(request, "Cookie", "HTTP_COOKIE", buf, param, index++);
    set_envp_field_with_header(request, "User-Agent", "HTTP_USER_AGENT", buf, param, index++);
    set_envp_field_with_header(request, "Host", "HTTP_HOST", buf, param, index++);
    set_envp_field_with_header(request, "Connection", "HTTP_CONNECTION", buf, param, index++);

    set_envp_field_by_str("REMOTE_ADDR", hap.host, buf, param, index++);
    char port_str[8];
    memset(port_str, 0, 8);
    sprintf(port_str, "%d", hap.port);
    set_envp_field_by_str("SERVER_PORT", port_str, buf, param, index++);
    param->envp[index++] = NULL;
}

void print_CGI_param(CGI_param *param) {
    CGI_LOG("%s", "-------CGI Param------")
    CGI_LOG("Filename]: %s", param->filename)
    CGI_LOG("[Args]: %s", param->args[0])
    CGI_LOG("%s", "Envp]:")
    int i = 0;
    while (param->envp[i] != NULL) {
        CGI_LOG("%d", i + 1)
        CGI_LOG("%s", param->envp[i])
        i++;
    }
    CGI_LOG("%s", "-------End CGI Para------")
}

char *new_string(char *str) {
    char *buf = (char *) malloc(strlen(str) + 1);
    memcpy(buf, str, strlen(str) + 1);
    return buf;
}

void set_envp_field_by_str(char *envp_name, char *value, char *buf, CGI_param *param, int index) {
    memset(buf, 0, CGI_ENVP_INFO_MAXLEN);
    sprintf(buf, "%s=%s", envp_name, value);
    param->envp[index] = new_string(buf);
}

void set_envp_field_with_header(Request *request, char *header, char *envp_name, char *buf, CGI_param *param,
                                int index) {
    char *value = get_header_value(request, header);
    memset(buf, 0, CGI_ENVP_INFO_MAXLEN);
    sprintf(buf, "%s=%s", envp_name, value);
    param->envp[index] = new_string(buf);
}

//void print_executor(CGI_executor *executor) {
//    CGI_LOG("%s", "------Executor Info-------")
//    CGI_LOG("Client socket: %d", executor->clientfd);
//    CGI_LOG("CGI socket to read from %d", executor->stdout_pipe[0])
//    CGI_LOG("CGI socket to write to %d", executor->stdin_pipe[1])
//    CGI_LOG("%s", "Buffer state")
//    CGI_LOG("-- Buffer Offset: %d", executor->cgi_buffer->data)
//    CGI_LOG("-- Buffer Capacity: %d", executor->cgi_buffer->data)
//    CGI_LOG("-- Buffer Content: %s", executor->cgi_buffer->data)
//    CGI_LOG("%s", "-----End Executor Info-----")
//}

void execve_error_handler() {
    switch (errno) {
        case E2BIG:
            CGI_LOG("%s",
                    "The total number of bytes in the environment (envp) and argument list (argv) is too large.")
            return;
        case EACCES:
            CGI_LOG("%s", "Execute permission is denied for the file or a script or ELF interpreter.")
            return;
        case EFAULT:
            CGI_LOG("%s", "ilename points outside your accessible address space.")
            return;
        case EINVAL:
            CGI_LOG("%s",
                    "iAn ELF executable had more than one PT_INTERP segment (i.e., tried to name more than one interpreter).")
            return;
        case EIO:
            CGI_LOG("%s", "An I/O error occurred.")
            return;
        case EISDIR:
            CGI_LOG("%s", "An ELF interpreter was a directory.")
            return;
//        case ELIBBAD:
//            CGI_LOG("%s", "An ELF interpreter was not in a recognised format.")
//            return;
        case ELOOP:
            CGI_LOG("%s",
                    "Too many symbolic links were encountered in resolving filename or the name of a script or ELF interpreter.")
            return;
        case EMFILE:
            CGI_LOG("%s", "The process has the maximum number of files open.")
            return;
        case ENAMETOOLONG:
            CGI_LOG("%s", "filename is too long.")
            return;
        case ENFILE:
            CGI_LOG("%s", "The system limit on the total number of open files has been reached.")
            return;
        case ENOENT:
            CGI_LOG("%s",
                    "The file filename or a script or ELF interpreter does not exist, or a shared library needed for file or interpreter cannot be found.")
            return;
        case ENOEXEC:
            CGI_LOG("%s",
                    "An executable is not in a recognised format, is for the wrong architecture, or has some other format error that means it cannot be executed.")
            return;
        case ENOMEM:
            CGI_LOG("%s", "Insufficient kernel memory was available.")
            return;
        case ENOTDIR:
            CGI_LOG("%s",
                    "A component of the path prefix of filename or a script or ELF interpreter is not a directory.")
            return;
        case EPERM:
            CGI_LOG("%s",
                    "The file system is mounted nosuid, the user is not the superuser, and the file has an SUID or SGID bit set.")
            return;
        case ETXTBSY:
            CGI_LOG("%s", "Executable was open for writing by one or more processes.")
            return;
        default:
            CGI_LOG("%s", "Unkown error occurred with execve().")
            return;
    }
}

void start_CGI_script(peer_t* client, CGI_param *cgi_parameter, char *post_body, size_t content_length) {

    CGI_LOG("%s", "In handle_dynamic_request(), creating new cgi_executor")
    assert(client->cgi_executor == NULL); // the cgi_executor should be NULL at this point
    client->cgi_executor = init_CGI_executor();
    if (content_length > 0) {
        client->cgi_executor->cgi_buffer = (cbuf_t *) malloc(sizeof(cbuf_t));
        buf_write(client->cgi_executor->cgi_buffer, (uint8_t*) post_body, content_length);
    }
    client->cgi_executor->cgi_parameter = cgi_parameter;
    CGI_LOG("%s", "Creating new cgi_executor is done")

    pid_t pid;
    // pipe(int[2]):  if successful, the array will contain two new file descriptors to be used for the pipeline
    if (pipe(client->cgi_executor->stdin_pipe) < 0) {
        CGI_LOG("[Fatal] Error piping for stdin for cliend %d", client->socket)
        assert(false);
    }

    if (pipe(client->cgi_executor->stdout_pipe) < 0) {
        CGI_LOG("[Fatal] Error piping for stdin for cliend %d", client->socket)
        assert(false);
    }

    pid = fork();
    if (pid < 0) {
        CGI_LOG("[Fatal] Error forking child CGI process for client %d", client->socket)
        assert(false);
    }

    if (pid == 0) {     /* Child CGI process */
        close(client->cgi_executor->stdout_pipe[0]);
        dup2(client->cgi_executor->stdout_pipe[1], fileno(stdout)); // for write
        close(client->cgi_executor->stdin_pipe[1]);
        dup2(client->cgi_executor->stdin_pipe[0], fileno(stdin)); // for read

        /* Execute CGI Script */
        if (execve(client->cgi_executor->cgi_parameter->filename,
                   client->cgi_executor->cgi_parameter->args,
                   client->cgi_executor->cgi_parameter->envp)) {
            execve_error_handler();
            CGI_LOG("[Fatal] Error executing CGI script for cliend %d", client->socket)
            assert(false);
        }
    }

    if (pid > 0) {
        // parent process write to stdin_pipe[1] to send bytes to child process
        // parent process read from stdout_pipe[0] to receive bytes to child process
        close(client->cgi_executor->stdout_pipe[1]);
        close(client->cgi_executor->stdin_pipe[0]);
    }
}

// Get sockaddr, IPv4 or IPv6.
void *get_in_addr(sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((sockaddr_in *) sa)->sin_addr);
    }
    return &(((sockaddr_in6 *) sa)->sin6_addr);
}
