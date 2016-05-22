#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char *argv[]) {
    if (argc - 1 < 1) {
        errx(1, "not enough arguments");
    }
    ssize_t size;
    for (int i = 1; i < argc; ++i) {
        size_t n = 0;
        if (argv[i][0] == '@' && argv[i][1] == '\0') {
            if ((size = getline(argv + i, &n, stdin)) == -1) {
                err(1, NULL);
            } else if (size > 0) {
                argv[i][size - 1] = '\0';
            }
        }
    }
    return execvp(argv[1], argv + 1);
}

