// gpp self $cppflags

#include <err.h>
#include <stdio.h>
#include <stdlib.h>

// cpp

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

extern char *__progname;

static void usage();

int main(int argc, char *argv[]) {
    if (argc - 1 < 2 || argc - 1 > 3) {
        usage();
        errx(1, "wrong number of arguments");
    }
    bool cont = argc - 1 == 3;
    auto var = std::string(argv[1]);
    auto sub = std::string(argv[2]);
    if (cont && std::string(argv[3]) != "-c") {
            usage();
            errx(1, "unrecognised flag");
    }
    auto it = std::begin(sub);
    std::vector<decltype(it)> vars;
    while ((it = std::search(it, std::end(sub), std::begin(var),
                             std::end(var))) != std::end(sub)) {
        vars.push_back(it);
        it += var.size();
    }
    if (vars.empty()) {
        usage();
        errx(1, "var not in cmd");
    }
    int ret;
    for (std::string s, cmd = sub; std::getline(std::cin, s);) {
        cmd.resize(0);
        it = std::begin(sub);
        for (const auto &et : vars) {
            cmd += std::string(it, et) + s;
            it = et + var.size();
        }
        cmd += std::string(it, std::end(sub));
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
