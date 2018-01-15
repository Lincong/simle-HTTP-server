//
// Created by Lincong Li on 1/13/18.
//

#include "file_handlers.h"


bool is_dir(const char *path)
{
    struct stat sb;
    if(stat(path, &sb) != 0 ) {
        fprintf(stderr, "Error with the path\n");
        return false;
    }
    return S_ISDIR(sb.st_mode);
}

bool is_regular_file(const char *path)
{
    struct stat sb;
    return (stat(path, &sb) == 0 && S_ISREG(sb.st_mode));
}

/*
 * Check whether a file path is directory.
 * Return 1 if it is, 0 otherwise.
 */
int requested_path_is_dir(const char *path) {
    size_t len = strlen(path);
    if (path[len-1] == '/') {
        return 1;
    }
    return 0;
}

/*
 * Wrapper for mkdir sys call.
 * If the directory already exists, do nothing.
 */
int create_folder(const char *path, mode_t mode) {
    int ret;
    struct stat st = {0};

    if (stat(path, &st) != -1) return 0;    /* If already exists then return */

    ret = mkdir(path, mode);
    if (ret < 0) {
        printf("mkdir error");
    }
    return ret;
}

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

/*
 * Convert all chars of a str to lower case in place.
 */
void str_tolower(char *str) {
    int i;
    for (i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

/*
 * Get file content length given file path.
 */
size_t get_file_len(const char* fullpath) {
    struct stat st;
    stat(fullpath, &st);
    return st.st_size;
}

/*
 * Get current time.
 */
void get_curr_time(char *time_buf, size_t buf_size) {
    time_t raw_time;
    struct tm * timeinfo;

    time(&raw_time);
    timeinfo = localtime(&raw_time);
    strftime(time_buf, buf_size, "%a, %d %b %Y %H:%M:%S %Z", timeinfo);
}

/*
 * Get file's last modified name given file path.
 */
void get_flmodified(const char*path, char *last_mod_time, size_t buf_size) {
    struct stat st;
    struct tm *curr_gmt_time = NULL;
    stat(path, &st);
    curr_gmt_time = gmtime(&st.st_mtime);
    strftime(last_mod_time, buf_size, "%a, %d %b %Y %H:%M:%S %Z", curr_gmt_time);
}