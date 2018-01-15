//
// Created by Lincong Li on 1/13/18.
//

#ifndef FILE_HANDLERS_H
#define FILE_HANDLERS_H

#include "common.h"
#include <sys/stat.h>
#include <ctype.h>

bool is_dir(const char *path);
bool is_regular_file(const char *path);
int requested_path_is_dir(const char *path);
void get_extension(const char *, char*);
const char *get_filename_ext(const char *filename);
void str_tolower(char *);
void get_curr_time(char *, size_t);
size_t get_file_len(const char*);
void get_flmodified(const char*, char *, size_t);

#endif //SRC_FILE_HANDLERS_H
