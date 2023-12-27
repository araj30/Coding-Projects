#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SOMETHING 4096

// citation: https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
int fileinit(const char *fpath) {
    struct stat path_stat;
    stat(fpath, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int main() {
    char comm[PATH_MAX], filename[PATH_MAX];
    scanf("%s", comm);

    int result;
    int result2;

    result = strcmp(comm, "get");
    result2 = strcmp(comm, "set");

    if (result == 0) {
        scanf("%s", filename);
        //(void) filename;

        if (fileinit(filename) == 0 || strlen(filename) == 0) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        char check;
        scanf("%c", &check);

        if (check != '\n') {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        char another;
        //scanf("%c", &another);
        // why does this not work if scanf is outside?
        if (scanf("%c", &another) != EOF) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        int fd = open(filename, O_RDONLY);

        if (fd < 0) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        int bytes_read = 0;
        char buf[SOMETHING];

        // professoer quinn helped out with the byte reading logic in classs
        do {
            bytes_read = read(fd, buf, SOMETHING);
            if (bytes_read < 0) {
                fprintf(stderr, "Operation Failed\n");
                return 1;
            } else if (bytes_read > 0) {
                int bytes_written = 0;
                do {
                    int bytes
                        = write(STDOUT_FILENO, buf + bytes_written, bytes_read - bytes_written);
                    if (bytes <= 0) {
                        fprintf(stderr, "Operation Failed\n");
                        return 1;
                    }

                    bytes_written += bytes;
                } while (bytes_written < bytes_read);
            }
        } while (bytes_read > 0);
        close(fd);
        return 0;
    } else if (result2 == 0) {
        scanf("%s", filename);

        if (strlen(filename) == 0) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        int sz = open(filename, O_TRUNC | O_CREAT | O_WRONLY, 0644);

        if (sz < 0) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        int length = 0;

        scanf("%d", &length);

        if (length == 0) {
            printf("OK\n");
            close(sz);
            return 0;
        }

        int bytes_read = 0;
        char buf[SOMETHING];

        int hold = length - SOMETHING;

        // quinn helped w byte reading logic in class
        do {
            if (hold < 0) {
                bytes_read = read(STDIN_FILENO, buf, (hold + SOMETHING));
            } else {
                bytes_read = read(STDIN_FILENO, buf, SOMETHING);
            }
            if (bytes_read < 0) {
                fprintf(stderr, "Operation Failed\n");
                close(sz);
                return 1;
            } else if (bytes_read > 0) {
                int bytes_written = 0;
                do {
                    int bytes = write(sz, buf + bytes_written, bytes_read - bytes_written);
                    if (bytes <= 0) {
                        fprintf(stderr, "Invalid Command\n");
                        close(sz);
                        return 1;
                    }
                    bytes_written += bytes;
                } while (bytes_written < bytes_read);
            }
        } while (bytes_read > 0);

        printf("OK\n");
        close(sz);
        return 0;

    } else {
        fprintf(stderr, "Invalid Command\n");
        return 1;
    }
}
