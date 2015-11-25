// gpp self

#include <vector>
#include <string>
#include <stdio.h>
#include <string.h>

using namespace std;

struct str {
    char* leak;
    size_t n;
};

static vector<str> vstr;

void print(const vector<str>& vstr, size_t n, bool transpose=false) {
    size_t nr = vstr.size()%n ? vstr.size()/n+1 : vstr.size()/n;
    vector<size_t> vmax(n, 0);
    if (!transpose) {
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < nr && j*n+i < vstr.size(); ++j) {
                if (vstr[j*n+i].n > vmax[i]) {
                    vmax[i] = vstr[j*n+i].n;
                }
            }
        }
        for (size_t i = 0; i < nr; ++i) {
            for (size_t j = 0; j < n && i*n+j < vstr.size(); ++j) {
                printf("%-*s", (int)vmax[j], vstr[i*n+j].leak);
            }
            printf("\n");
        }
    } else {
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < nr && i*nr+j < vstr.size(); ++j) {
                if (vstr[i*nr+j].n > vmax[i]) {
                    vmax[i] = vstr[i*nr+j].n;
                }
            }
        }
        for (size_t i = 0; i < nr; ++i) {
            for (size_t j = 0; j < n && j*nr+i < vstr.size(); ++j) {
                printf("%-*s", (int)vmax[j], vstr[j*nr+i].leak);
            }
            printf("\n");
        }
    }
}

// Non-lazy sink, presents lines in a tabular form, provided the number of
// columns

int main(int argc, char* argv[]) {
    auto print_help = [&]() {
        fprintf(stderr, "USAGE:\t%s n [-t]\n"
        "SCOPE:\tpresent lines in a tabular form [line; n]\n"
        "\t\twhen n is 0 returns 0\n", argv[0]);
    };
    if (argc-1 < 1) {
        print_help();
        return 1;
    }
    size_t n = stoul(argv[1]);
    ssize_t s;
    bool transpose = false;
    str in{NULL, 0};
    if (n == 0) {
        return 0;
    }
    if (argc-1 == 2) {
        if (strcmp(argv[2], "-t") == 0) {
            transpose = true;
        } else {
            print_help();
            return 1;
        }
    }
    while((s = getline(&in.leak, &in.n, stdin)) != -1) {
        in.n = s;
        in.leak[s-1] = ' ';
        vstr.push_back(in);
        in.leak = NULL;
        in.n = 0;
    }
    print(vstr, n, transpose);
    return 0;
}
