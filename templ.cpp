// gpp self

#include <string>
#include <iostream>
#include <stdio.h>
#include <err.h>

/*
 * Tiny templating program
 * <html><body>
 * # import=cat bash
 * #    $import page.html
 * #    for i in *; do
 * #      echo "  <a href=\"$PWD/$i\">$i</a>"
 * #    done
 * </body></html>
 * #python
 * #from datetime import date
 * #print(date.today())
 */

int main(int argc, char *argv[]) {
    FILE *exec;
    std::string cmd, in, env;
    char quot = (argc-1) ? argv[1][0] : '#';
    auto call_once = [&]() {
        if (env.empty()) {
            return;
        }
        if ((exec = popen(env.c_str(), "w")) == NULL) {
            err(1, "%s", env.c_str());
        }
        if (!cmd.empty() &&
            fwrite(cmd.c_str(), 1, cmd.size(), exec) != cmd.size()) {
            (void)pclose(exec);
            err(1, "%s", env.c_str());
        }
        if (pclose(exec) != 0) {
            errx(1, "%s failed", cmd.c_str());
        }
        cmd.resize(0);
        env.resize(0);
    };
    while (std::getline(std::cin, in)) {
        if (in.empty() || in[0] != quot) {
            call_once();
            printf("%s\n", in.c_str());
        } else {
            if (env.empty()) {
                env = std::string(in.data()+1);
            } else {
                cmd += std::string(in.data()+1) + '\n';
            }
        }
    }
    call_once();
    return 0;
}
