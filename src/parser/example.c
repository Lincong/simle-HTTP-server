/* C declarations used in actions */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "parse.h"

void print_parse_buf(char* curr_task, int size);

int main(int argc, char **argv){
  //Read from the file the sample
  int index;
  char buf[8192];

    int i;
    for(i = 0; i < 20; i++) {
        printf("---------- begin --------------\n");
        int fd_in = open(argv[1], O_RDONLY);
        if(fd_in < 0) {
            printf("Failed to open the file\n");
            return 0;
        }
        int readRet = read(fd_in, buf, 8192);
        close(fd_in);
        //Parse the buffer to the parse function. You will need to pass the socket fd and the buffer would need to
        //be read from that fd
        print_parse_buf(buf, readRet);

        Request *request = parse(buf, readRet);
        //Just printing everything
        printf("Http Method: %s\n", request->http_method);
        printf("Http Version: %s\n", request->http_version);
        printf("Http Uri: %s\n", request->http_uri);
        for (index = 0; index < request->header_count; index++) {
            printf("Request Header\n");
            printf("Header name: %s\nHeader Value: %s\n\n", request->headers[index].header_name,
                   request->headers[index].header_value);
        }
        free(request->headers);
        free(request);
        printf("---------- end --------------\n");
    }
  return 0;
}

void print_parse_buf(char* curr_task, int size)
{
    printf("Parse buffer:\n");
    int i;
    for(i = 0; i < size; i++)
        printf("%c", curr_task[i]);
    printf("Parse buffer ends\n");
}
