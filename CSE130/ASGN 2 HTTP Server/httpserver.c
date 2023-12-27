#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <regex.h>
#include <string.h>
#include <unistd.h>
#include "asgn2_helper_funcs.h"

#define RSIZE 2048
#define BSIZE 5000

// perform get operations
int get(int fd, char *uri) {
    // buffer to hold errors codes
    char errors[BSIZE];

    // stores info from file
    struct stat file_check;

    int d;

    // error codes
    if (stat(uri, &file_check) == -1) {
        if (errno == ENOENT) {
            sprintf(errors, "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n");
            write_n_bytes(fd, errors, 90);
            return 1;

        } else if (errno == EACCES) {
            sprintf(errors, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
            write_n_bytes(fd, errors, 90);
            return 1;

        } else {
            sprintf(errors,
                "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal "
                "Server Error\n");
            write_n_bytes(fd, errors, 90);
            return 1;
        }
    }

    // if directory, originally had this above error codes, but it wasn't producing correct
    if (S_ISDIR(file_check.st_mode) != 0) {
        sprintf(errors, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
        write_n_bytes(fd, errors, 90);
        return 1;
    }

    d = open(uri, O_RDONLY);

    // if any errors in opening file, throw error
    if (d == -1) {
        if (errno == EACCES) {
            sprintf(errors, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
            write_n_bytes(fd, errors, 90);
            return 1;
        }
    }

    // successful
    sprintf(errors, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n", file_check.st_size);

    // decided to use strlen here since we don't know the file size
    write_n_bytes(fd, errors, strlen(errors));
    pass_n_bytes(d, fd, file_check.st_size);

    close(d);
    return 0;
}

// perform put operations
int put(int fd, char *uri, int reading_input, int content_length, char buffer[BSIZE + 1],
    char message_buffer[BSIZE]) {
    // buffer for error codes
    char errors[BSIZE];

    // holds info about file
    struct stat file_check;

    int exists = 0;
    int valid = stat(uri, &file_check);

    int num;
    int d;

    if (valid == 0) {
        exists = 1;
    }

    d = open(uri, O_CREAT | O_WRONLY | O_TRUNC, 0644);

    // if any errors in opening file, throw error
    if (d == -1) {
        if (errno == EACCES) {
            sprintf(errors, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
            valid = write_n_bytes(fd, errors, 90);
        }
        return 1;
    }

    // calculating num bytes to write into d w helper function
    num = (int) (reading_input + buffer - message_buffer);
    valid = write_n_bytes(d, message_buffer, num);

    // passing bytes w helper function
    pass_n_bytes(fd, d, content_length - num);

    if (exists == 1) {
        sprintf(errors, "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n");
        valid = write_n_bytes(fd, errors, 90);
    } else {
        sprintf(errors, "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n");
        valid = write_n_bytes(fd, errors, 90);
    }

    close(d);
    return 0;
}

int parse(int fd) {
    // help from class and mitchell for regex
    regmatch_t comp_reg[20];
    regex_t regex;

    // all for request and message
    char *method;
    char *uri;
    char *version;
    char *heads;
    char *key;
    char *value;
    char *message_buffer;
    char *msg;

    // buff for error codes
    char errs[BSIZE];
    char buffer[BSIZE + 1];

    int check = 0;
    int size = RSIZE;
    int reading_input = 0;
    int total = 0;
    int content_length;

    while (total < size) {
        // read the input from user and decrement how much you have left to read
        reading_input = read(fd, buffer + total, size - total);

        // no more bytes to read or error in reading (respectively)
        if (reading_input == 0) {
            break;

        } else if (reading_input == -1) {
            sprintf(errs, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
            check = write_n_bytes(fd, errs, 100);
            return 1;
        }
        total += reading_input;

        // null terminate string
        buffer[total] = '\0';
        // check if the delimiter is contained in the buffer
        msg = strstr(buffer, "\r\n\r\n");

        // if in buffer, store the message in message_buffer
        if (msg != NULL) {
            message_buffer = msg + 4;
            break;
        }
    }

    // making the request a null terminated string
    buffer[reading_input] = '\0';

    // parsing help from class and mitchell
    // check if if regex matches the requset format, compare
    check = regcomp(
        &regex, "([a-zA-Z]{1,8}) /([a-zA-Z0-9.]{2,64}) (HTTP/[0-9].[0-9])\r\n", REG_EXTENDED);

    // further defines regex, buffer is the string we're checking, there are 4 sections we're chcking for,
    // match is the array that holds each part to compare, 0 is just standard comparison
    check = regexec(&regex, (char *) buffer, 4, comp_reg, 0);

    // if not regex matched, error
    // i preferred doing a number instead of strlen, but hardcoding the number is giving me errors for all the following cases
    // so i decided to use strlen instead for the following
    if (check != 0) {
        sprintf(errs, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
        // strlen was the only thing that worked here
        check = write_n_bytes(fd, errs, strlen(errs));
        regfree(&regex);
        return 1;
    }

    // assigns the regex matching to their checkpective strings and then checkets regex matching to null
    // offsetting buffer so we read in exactly what we need... each part of command
    method = buffer + comp_reg[1].rm_so;
    uri = buffer + comp_reg[2].rm_so;
    version = buffer + comp_reg[3].rm_so;

    // null terminating all sections
    buffer[comp_reg[1].rm_eo] = '\0';
    buffer[comp_reg[2].rm_eo] = '\0';
    buffer[comp_reg[3].rm_eo] = '\0';

    // if version is not correct, write error
    if ((strcmp(version, "HTTP/1.1")) != 0) {
        sprintf(errs, "HTTP/1.1 505 Version Not Supported\r\nContent-Length: "
                      "22\r\n\r\nVersion Not Supported\n");
        // strlen was the only thing that worked here
        check = write_n_bytes(fd, errs, strlen(errs));
        return 1;
    }
    check = regcomp(&regex, "(([a-zA-Z0-9.-]{1,128}): ([ -~]{0,128}))?", REG_EXTENDED);

    // offsets buffer to beginning of header fields (if any)
    heads = buffer + comp_reg[3].rm_eo + 2;

    // if the header field is \r\n, then keep on reading
    while (heads[0] != '\r' || heads[1] != '\n') {

        // doing check for format of header fields
        check = regexec(&regex, heads, 20, comp_reg, 0);
        if (check != 0) {
            sprintf(errs, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
            // only strlen worked here
            check = write_n_bytes(fd, errs, strlen(errs));
            regfree(&regex);
            return 1;
        }

        // parse header into key and value
        key = heads + comp_reg[2].rm_so;
        value = heads + comp_reg[3].rm_so;

        // null terminate the strings
        heads[comp_reg[2].rm_eo] = '\0';
        heads[comp_reg[3].rm_eo] = '\0';

        // if the content length is less than or equal to zero, throw an error
        content_length = atoi(value);
        if (strcmp(key, "Content-Length") == 0 && content_length <= 0) {
            sprintf(errs, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
            // only strlen worked here
            check = write_n_bytes(fd, errs, strlen(errs));
            regfree(&regex);
            return 1;
        }
        // move to new header line
        heads = heads + comp_reg[3].rm_eo + 2;
    }

    regfree(&regex);

    // check what method we need to use
    if (strcmp(method, "GET") == 0) {
        get(fd, uri);
    } else if (strcmp(method, "PUT") == 0) {
        put(fd, uri, reading_input, content_length, buffer, message_buffer);
    } else {
        sprintf(
            errs, "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n");
        // only strlen worked here
        check = write_n_bytes(fd, errs, strlen(errs));
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int port_num;
    int init;

    Listener_Socket socket;

    // checking if the user entered two args
    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments\n");
        exit(1);
    }

    // checking for valid port number
    else if (atoi(argv[1]) > 65535 || atoi(argv[1]) < 1) {
        fprintf(stderr, "Invalid Port\n");
        exit(1);
    }

    port_num = atoi(argv[1]);
    init = listener_init(&socket, port_num);

    // if can't listen from socket
    if (init == -1) {
        write(STDERR_FILENO, "Failed to listen\n", strlen("Failed to listen\n"));
        return 1;
    }

    // keep socket open for connections
    while (1) {
        int file = listener_accept(&socket);

        if (file == -1) {
            write(STDERR_FILENO, "Failed to accept\n", strlen("Failed to accept\n"));
            return 1;
        }

        init = parse(file);
        close(file);
    }
    return 0;
}
