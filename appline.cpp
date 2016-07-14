// gpp self $cppflags

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

// cpp

#include <algorithm>
#include <iostream>
#include <string>

using namespace std;

extern char *__progname;

static void usage();

int main(int argc, char *argv[]) {
    if (argc - 1 < 2 || argc - 1 > 3) {
        usage();
        errx(1, "wrong number of arguments");
    }
    bool cont = argc - 1 == 3;
    auto arg1 = std::string(argv[1]);
    auto arg2 = std::string(argv[2]);
    if (argc - 1 == 3) {
        auto arg3 = std::string(argv[3]);
        if (arg3 != "-c") {
            usage();
            errx(1, "unrecognised flag");
        }
    }
    auto it = std::search(std::begin(arg2), std::end(arg2), std::begin(arg1),
                          std::end(arg1));
    if (it == std::end(arg2)) {
        usage();
        errx(1, "var not in cmd");
    }
    auto fc = std::string(std::begin(arg2), it),
         sc = std::string(it + arg1.size(), std::end(arg2)), cmd = fc;
    int ret;
    for (std::string s; std::getline(std::cin, s);) {
        cmd = fc + s + sc;
        if ((ret = system(cmd.c_str())) != 0 && !cont) {
            errx(ret, "%s", cmd.c_str());
        }
    }
    return 0;
}

static void
usage() {
    fprintf(stderr, "Usage: %s var cmd [-c]"
                    "\n\tvar<string>: variable name"
                    "\n\tcmd<string>: shell command"
                    "\n\t-c: continue on error"
                    "\nScope: executes the command cmd with var replaced by"
                    " lines from stdin"
                    "\n",
            __progname);
}
