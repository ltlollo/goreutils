// gpp self

#include <string>

using namespace std;

int launch(const string& cmd) noexcept {
    return !cmd.empty() ? system(cmd.c_str()) : 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s expr\n", argv[0]);
        return 2;
    }
    char* cmd_beg = argv[1], *cmd_it = argv[1];
    string cmd_str;
    int ret;

    for (;;) {
        if (*cmd_it == '\0' || *cmd_it == ';' || *cmd_it  == '\n') {
            cmd_str = string(cmd_beg, cmd_it);
            ret = launch(cmd_str);
            if (*cmd_it == '\0' || ret) {
                return ret;
            }
            cmd_beg = ++cmd_it;
        } else {
            ++cmd_it;
        }
    }
}

