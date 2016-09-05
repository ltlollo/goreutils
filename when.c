// gpp self $cppflags

#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/inotify.h>
#include <unistd.h>

extern char *__progname;

static void help(void);

int
main(int argc, char *argv[]) {
    int fd, wd;
    ssize_t rd;
    char buf[4096];
    uint32_t evm = IN_ALL_EVENTS;
    if (argc < 2 || argc > 3) {
        help();
        errx(1, "not enough arguemnts");
    }
    if (argc == 3) {
        evm = 0;
        for (char *l = argv[2]; *l; ++l) {
            switch (*l) {
            case 'a':
                evm |= IN_ACCESS;
                break;
            case 'r':
                evm |= IN_ATTRIB;
                break;
            case 'w':
                evm |= IN_CLOSE_WRITE;
                break;
            case 'n':
                evm |= IN_CLOSE_NOWRITE;
                break;
            case 'o':
                evm |= IN_OPEN;
                break;
            case 'f':
                evm |= IN_MOVED_FROM;
                break;
            case 't':
                evm |= IN_MOVED_TO;
                break;
            case 'd':
                evm |= IN_DELETE;
                break;
            case 's':
                evm |= IN_DELETE_SELF;
                break;
            case 'c':
                evm |= IN_MODIFY;
                break;
            default:
                help();
                errx(1, "unknown option argument");
            }
        }
    }
    if ((fd = inotify_init()) == -1) {
        err(1, "inotify_init");
    }
    if ((wd = inotify_add_watch(fd, argv[1], evm)) == -1) {
        err(1, "inotify_add_watch");
    }
RETRY:
    if ((rd = read(fd, buf, sizeof(buf))) < 0) {
        if (errno == EINTR) {
            goto RETRY;
        }
        err(1, "read");
    }
    return 0;
}

static void
help(void) {
    fprintf(stderr, "Usage:\t%s file [attrs]"
                    "\n\tfile<string>: file name"
                    "\n\tattrs:"
                    "\n\t\ta: IN_ACCESS"
                    "\n\t\tr: IN_ATTRIB"
                    "\n\t\tw: IN_CLOSE_WRITE"
                    "\n\t\tn: IN_CLOSE_NOWRITE"
                    "\n\t\to: IN_OPEN"
                    "\n\t\tf: IN_MOVED_FROM"
                    "\n\t\tt: IN_MOVED_TO"
                    "\n\t\td: IN_DELETE"
                    "\n\t\ts: IN_DELETE_SELF"
                    "\n\t\tc: IN_MODIFY"
                    "\nScope:\tmonitors file changes, returns 0 when an event "
                    "happens"
                    "\n",
            __progname);
}
