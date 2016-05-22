#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern char *__progname;

static void usage(void);

int
main(int argc, char *argv[]) {
    FILE *exec = NULL;
    char quot = '#';
    char *line = NULL;
    size_t len = 0;
    size_t lineno = 0;
    ssize_t read;
    int opt;
    int ret;
    while ((opt = getopt(argc, argv, "hq:i:o:")) != -1) {
        switch (opt) {
        case 'i':
            if (freopen(optarg, "r", stdin) == NULL) {
                err(1, "freopen");
            }
            break;
        case 'o':
            if (freopen(optarg, "w", stdout) == NULL) {
                err(1, "freopen");
            }
            break;
        case 's':
            if (strlen(optarg) != 1) {
                errx(1, "quote must be a byte");
            } else {
                quot = *optarg;
            }
            break;
        case 'h':
            usage();
            return 0;
        }
    }
    while ((read = getline(&line, &len, stdin)) != -1) {
        if (read != 0 && line[0] == quot) {
            if (exec == NULL) {
                if ((exec = popen(line + 1, "w")) == NULL) {
                    goto FAIL;
                }
            } else {
                fprintf(exec, "%s", line + 1);
            }
        } else {
            if (exec != NULL) {
                if ((ret = pclose(exec)) == -1) {
                    goto FAIL;
                } else if (ret != 0) {
                    warnx("Warn - line %ld: %sstatus: %d\n", lineno, line,
                          ret);
                }
                exec = NULL;
            }
        }
        ++lineno;
    }
    if (exec != NULL) {
        if ((ret = pclose(exec)) == -1) {
            goto FAIL;
        } else if (ret != 0) {
            warnx("Warn - line %ld: %sstatus: %d\n", lineno, line, ret);
        }
    }
    free(line);
    return 0;
FAIL:
    free(line);
    err(1, "Err - line %ld: %s", lineno, line);
}

void
usage(void) {
    fprintf(stderr, "Usage:\t%s [-q Q][-o out][-i in][-h]"
                    "\nScope:\ttiny templating program: executes `Q` starting "
                    "\n\tblocks `Q CMD; (Q REST)...` as in `CMD` <<< `REST`..."
                    "\n\t-i in<string>: input file (default: stdin)"
                    "\n\t-o out<string>: output file (default: stdout)"
                    "\n\t-q Q<byte>: quotation symbol (default: '#')"
                    "\n\t-h: this message\n",
            __progname);
}
