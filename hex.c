// gpp self $cppflags

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#define MODEL                                                                 \
    "0000000000000000: 00 00 00 00 00 00 00 00 | 00 00 00 00 00 00 00 00 | "
#define FORMAT                                                                \
    "%016lx: %02x %02x %02x %02x %02x %02x %02x %02x | %02x %02x %02x %02x "  \
    "%02x %02x %02x %02x | "

static void usage(void);

extern char *__progname;

int
main(int argc, char *argv[]) {
    if (argc - 1 != 1) {
        usage();
        errx(1, "not enough arguments");
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        err(1, "open");
    }
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        err(1, "fstat");
    }
    size_t len = sb.st_size;
    uint8_t *addr = (uint8_t *)mmap(NULL, len, PROT_READ,
                                    MAP_PRIVATE | MAP_POPULATE, fd, 0);
    if (addr == MAP_FAILED) {
        err(1, "mmap");
    }
    char buf[sizeof(MODEL) + 16];
    char *real = buf + sizeof(MODEL) - 1;
    real[16] = '\n';
    for (size_t i = 0; i < len / 16; ++i) {
        sprintf(buf, FORMAT, i, addr[0], addr[1], addr[2], addr[3], addr[4],
                addr[5], addr[6], addr[7], addr[8], addr[9], addr[10],
                addr[11], addr[12], addr[13], addr[14], addr[15]);
        for (unsigned i = 0; i < 16; ++i) {
            real[i] = isprint(*addr) ? *addr : ' ';
            addr++;
        }
        fwrite(buf, 1, sizeof(buf), stdout);
    }
    return 0;
}

static void
usage(void) {
    fprintf(stderr, "Usage:\t%s file"
                    "\n\tfile<string>:\tfile to open"
                    "\nScope:\tshows the hex dump of the file"
                    "\n",
            __progname);
}
