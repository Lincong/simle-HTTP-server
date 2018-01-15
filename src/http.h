//
// Created by Lincong Li on 1/12/18.
//

#ifndef HTTP_H
#define HTTP_H

#include "common.h"
#include "message.h"

#define CL '\r'
#define RF '\n'
#define CLRF ((char*)"\r\n")
#define SP ((char*)" ")
#define HTTP_VERSION ((char*)"HTTP/1.1")
#define COLON ((char*)":")
#define DEFAULT_INDEX_FILE ((char*)"index.html")

#define HEADER_TERM_STATUS 4
#define PARSER_BUF_OVERFLOW 2
#define EMPTY_RECV_BUF 3

// HTTP responses codes
#define OK_NUM 200
#define BAD_REQUEST_NUM 400
#define NOT_FOUND_NUM   404
#define LENGTH_REQUIRE_NUM 411
#define INTERNAL_SERVER_ERROR_NUM 500
#define NOT_IMPLEMENTED_NUM 501
#define HTTP_VERSION_NOT_SUPPORTED_NUM 505
#define HEADER_GENRATE_ERROR -1

#define CODE_200 ((char*)"200")
#define CODE_404 ((char*)"404")
#define CODE_400 ((char*)"400")
#define CODE_411 ((char*)"411")
#define CODE_500 ((char*)"500")
#define CODE_501 ((char*)"501")
#define CODE_505 ((char*)"505")

// HTTP response reasons
#define OK ((char*)"OK")
#define NOT_FOUND ((char*)"Not Found")
#define BAD_REQUEST ((char*)"Bad Request")
#define LENGTH_REQUIRE ((char*)"Length Require")
#define INTERNAL_SERVER_ERROR ((char*)"Internal Server Error")
#define NOT_IMPLEMENTED ((char*)"Not Implemented")
#define HTTP_VERSION_NOT_SUPPORTED ((char*)"HTTP Version not supported")

#define CONNECTION ((char*)"Connection")
#define CLOSE ((char*)"Close")
#define KEEP_ALIVE ((char*)"Keep-alive")
#define INDEX_FILE ((char*)"Index.html")

#define NO_METHOD 0
#define GET_METHOD 1
#define HEAD_METHOD 2
#define POST_METHOD 3

#define STREAM_FILE_NOT_DONE 0
#define STREAM_FILE_DONE  1
#define STREAM_FILE_ERROR 2

// mime type

#define TEXT_HTML "text/html"
#define TEXT_CSS "text/css"
#define CSS "css"
#define PNG "png"
#define IMAGE_PNG "image/png"
#define JPEG "jpeg"
#define IMAGE_JPEG "image/jpeg"
#define GIF "gif"
#define IMAGE_GIF "image/gif"
#define APPLICATION_OCTET_STREAM "application/octet-stream"

int handle_http(peer_t *peer);

#endif //HTTP_H
