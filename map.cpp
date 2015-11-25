// gpp self

#include <string>
#include <cstdlib>
#include <cstdio>

int main(int argc, char *argv[]) {
    auto it_arg = argv + 1;
    auto args_size = argc - 1;

    if (args_size < 2) {
        fprintf(stderr,
                "USAGE: %s f x [xs]\n"
                "SCOPE: executes f x [ && %s f xs]\n",
                argv[0], argv[0]);
        return 1;
    }

    auto ret = int(0);
    auto cmd = it_arg[0];
    std::string cmdline;
    for(auto arg_end = (it_arg++) + args_size; it_arg != arg_end; ++it_arg) {
        cmdline = cmd;
        cmdline.push_back(' ');
        cmdline.insert(cmdline.size(), *it_arg);
        ret = std::system(cmdline.c_str());
        if (ret != 0) {
            fprintf(stderr, "fail: %s\n", cmdline.c_str());
            return ret;
        }
    }

    return 0;
}

