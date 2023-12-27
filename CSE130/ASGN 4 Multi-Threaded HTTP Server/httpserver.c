#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/file.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>
#include <errno.h>
#include <stdlib.h>

#include "request.h"
#include "queue.h"
#include "connection.h"
#include "response.h"
#include "rwlock.h"
#include "asgn2_helper_funcs.h"

#define integerToVoidPointer(i) (void *) (uintptr_t) (i)
#define voidPointerToInteger(v) (int) (uintptr_t) (v)
#define OPTIONS                 "t:h"
#define TEMP_FILNAME            ".temp_file"

struct stat info;
queue_t *rq;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *startingRoutine(void *queue);
void put_func(conn_t *);
void get_func(conn_t *);
void audit_log(const char *request, char *uri, uint16_t response_code, char *request_id);
void unsupported_func(conn_t *);

int main(int argc, char *argv[]) {

    rwlock_t *rw_lock;

    // int flag for parsing input- changes to -1 once there's no more left to read
    int get = 0;

    // default number of threads if not given
    int num = 4;

    while ((get = getopt(argc, argv, OPTIONS)) != -1) {
        switch (get) {
        // number of threads is invalid
        case 't':
            num = strtol(optarg, NULL, 10);
            if (num <= 0)
                errx(EXIT_FAILURE, "invalid thread count");
            break;
        // usage statement
        case 'h': fprintf(stderr, "Usage: %s [-t threads] <port>\n", argv[0]); return EXIT_SUCCESS;
        // unrecognized
        default: fprintf(stderr, "Usage: %s [-t threads] <port>\n", argv[0]); return EXIT_FAILURE;
        }
    }

    // not enough arguments
    if (optind >= argc) {
        fprintf(stderr, "wrong arguments: %s port_num", argv[0]);
        exit(EXIT_FAILURE);
    }

    // rest of main function was help from mitchell's ta sessions
    char *endptr = NULL;

    // convert string of input into a unsigned long long int value
    int port = (int) strtoull(argv[optind], NULL, 10);

    // if the end of the input is not null terminted
    if (endptr && *endptr != '\0') {
        warnx("invalid port number: %s", argv[1]);
        return EXIT_FAILURE;
    }

    // signal handler
    signal(SIGPIPE, SIG_IGN);

    // allowing program to listen for connections to port
    Listener_Socket listen;
    listener_init(&listen, port);

    // new queue initialized with the num of threads requested
    rq = queue_new(num);
    if (rq == NULL) {
        warnx("Queue can't be initialized");
        return EXIT_FAILURE;
    }

    // array for thread ids
    pthread_t *threads = calloc((num), sizeof(pthread_t));
    int i = 0;
    while (i < num) {
        // arguments: reference to ID of thread, attributes NULL, rando is starting routine, argument the starting routine takes
        int ls = pthread_create(&threads[i], NULL, &startingRoutine, rq);
        // if creating thread is not done successfully
        if (ls != 0) {
            return EXIT_FAILURE;
        }
        i++;
    }

    // Initialize reader-writer lock
    rw_lock = rwlock_new(N_WAY, 0);

    // pushing the accepted connections into queue
    while (1) {
        int connectionFileDescriptor = listener_accept(&listen);
        if (connectionFileDescriptor == -1) {
            continue;
        }

        // locks for pushing
        writer_lock(rw_lock);

        // pushing into queue now unlocked
        queue_push(rq, integerToVoidPointer(connectionFileDescriptor));
        writer_unlock(rw_lock);
    }

    // once done, delete the queue and return 0
    // freeing and deleting
    free(threads);
    queue_delete(&rq);
    rwlock_delete(&rw_lock);

    return 0;
}

void *startingRoutine(void *queue) {
    // pointer to what is being popped
    void *holdingPopped;

    // following logic for popping from mitchell

    // looping while true, continue popping of queue
    while (1) {
        queue_pop(queue, &holdingPopped);
        // conver the pointer to an int
        int conv = voidPointerToInteger(holdingPopped);
        conn_t *connection = conn_new(conv);

        // parsing connection
        const Response_t *res = conn_parse(connection);
        if (res == NULL) {
            const Request_t *req = conn_get_request(connection);

            // choose which function to call
            // handling connections from mitchell
            if (req == &REQUEST_PUT) {
                put_func(connection);
            } else if (req == &REQUEST_GET) {
                get_func(connection);
            } else {
                unsupported_func(connection);
            }
        } else {
            // in the case the response exits
            conn_send_response(connection, res);
        }
        // delete connection
        conn_delete(&connection);

        close(conv);
    }
}

void put_func(conn_t *connection) {
    // put logic from mitchell ta

    // connection for uri
    char *uri = conn_get_uri(connection);
    const Response_t *res = NULL;

    pthread_mutex_t put_file_mutex = PTHREAD_MUTEX_INITIALIZER;

    // unlock mutex
    pthread_mutex_lock(&mutex);

    // check if file exists
    bool exists = access(uri, F_OK) == 0;

    // try to open file, create/overwrite
    int fd = open(uri, O_CREAT | O_WRONLY, 0600);

    // error in opening file
    if (fd < 0) {
        // is directory, permission denied, or doesn't exist
        if (errno != EACCES && errno != ENOENT && errno != EISDIR) {
            pthread_mutex_unlock(&mutex);
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
        } else {
            pthread_mutex_unlock(&mutex);
            res = &RESPONSE_FORBIDDEN;
        }
        // print in audit log- action and error code
        audit_log("PUT", uri, response_get_code(res), conn_get_header(connection, "Request-Id"));
        conn_send_response(connection, res);
        return;
    }

    // mutex and lock management
    if (!exists) {
        // unlock mutex
        pthread_mutex_unlock(&mutex);
        // lock the lock
        pthread_mutex_lock(&put_file_mutex);
    } else {
        // lock the mutex
        pthread_mutex_lock(&put_file_mutex);
        //unlock lock
        pthread_mutex_unlock(&mutex);
    }

    // truncate file and get the file information
    ftruncate(fd, 0);
    res = conn_recv_file(connection, fd);

    // either response created or response ok (depending on it file existed before or not)
    if (res != NULL || !exists) {
        res = &RESPONSE_CREATED;
    } else if (res != NULL || exists) {
        res = &RESPONSE_OK;
    }

    // make entry on audit log
    audit_log("PUT", uri, response_get_code(res), conn_get_header(connection, "Request-Id"));

    // have to change this logic
    // releasing lock
    pthread_mutex_unlock(&put_file_mutex);

    // closing file and sending response
    close(fd);

    conn_send_response(connection, res);
    return;
}

void get_func(conn_t *connection) {
    // get logic from mitchell

    // connection for uri
    char *uri = conn_get_uri(connection);
    const Response_t *response = NULL;

    // mutex for the files specifically
    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

    // opening file as read only
    int fd = open(uri, O_RDONLY, 0666);

    // if there was an error in opening file, list error codes... if not, OK
    if (fd < 0) {
        if (errno == ENOENT) {
            response = &RESPONSE_NOT_FOUND;
        } else if (errno == EACCES) {
            response = &RESPONSE_FORBIDDEN;
        } else {
            response = &RESPONSE_INTERNAL_SERVER_ERROR;
        }
        if (response != &RESPONSE_OK) {
            conn_send_response(connection, response);
        }

        pthread_mutex_lock(&mutex);

        // make entry on audit log
        audit_log(
            "GET", uri, response_get_code(response), conn_get_header(connection, "Request-Id"));

        // unlock mutex
        pthread_mutex_unlock(&mutex);

        // close file
        close(fd);
        return;
    }

    // locking LOCK for get
    pthread_mutex_lock(&file_mutex);

    // file info
    fstat(fd, &info);

    // locking LOCK for get
    pthread_mutex_unlock(&file_mutex);

    // if file is in a directory
    if (S_ISDIR(info.st_mode)) {
        conn_send_response(connection, &RESPONSE_FORBIDDEN);
        close(fd);
        response = &RESPONSE_FORBIDDEN;

        // if file, lock up mutex and return to audit log
        pthread_mutex_lock(&mutex);
        audit_log(
            "GET", uri, response_get_code(response), conn_get_header(connection, "Request-Id"));
        pthread_mutex_unlock(&mutex);
        return;
    }

    // send file to client
    response = conn_send_file(connection, fd, info.st_size);
    if (response == NULL) {
        response = &RESPONSE_OK;
    }

    audit_log("GET", uri, response_get_code(response), conn_get_header(connection, "Request-Id"));

    // closing file
    close(fd);

    return;
}

void audit_log(const char *request, char *uri, uint16_t response_code, char *request_id) {
    fprintf(stderr, "%s,%s,%d,%s\n", request, uri, response_code, request_id);
}

void unsupported_func(conn_t *conn) {
    // from ta mitchell
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
}
