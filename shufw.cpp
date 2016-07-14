// gpp self $cppflags

#include <err.h>
#include <stdio.h>

// cpp

#include <algorithm>
#include <string>

extern char *__progname;

static void usage();

int
main(int argc, char *argv[]) {
    if (argc - 1 < 1 || argc - 1 > 2) {
        usage();
        errx(1, "wrong number of arguments");
    }
    std::string s = std::string(argv[1]) + "\n";
    if (argc - 1 == 2) {
        if (std::string(argv[2]) == "-s") {
            std::sort(std::begin(s), std::end(s) - 1);
        } else {
            usage();
            errx(1, "unrecognised option");
        }
    }
    auto size = s.size();
    do {
        if (fwrite(s.c_str(), 1, size, stdout) != size) {
            err(1, "fwrite");
        }
    } while (std::next_permutation(std::begin(s), std::end(s) - 1));
    return 0;
}

static void
usage() {
    fprintf(stderr, "Usage: %s in [-s]"
                    "\n\tin<string>: input word"
                    "\n\t-s: byte sort in"
                    "\nScope: print the permutations of in"
                    "\n",
            __progname);
}
