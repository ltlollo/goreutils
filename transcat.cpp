// gpp self

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <locale>
#include <codecvt>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    auto print_help = [&]() {
        fprintf(stderr, "Usage:\t%s -d dict [-i in][-o out][-s S][-1][-h]"
                "\nScope:\ttranslate S-prefixes into words from a dictionary"
                "\n\t-i in<string>: input file (default: stdin)"
                "\n\t-o out<string>: output file (default: stdout)"
                "\n\t-s S<rune>: prefix (default: @)"
                "\n\t-1: 1-based index (deafult: false)"
                "\n\t-h: this message\n", argv[0]);
    };
    std::locale::global(std::locale(""));
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::wstring sepa;
    wchar_t s = L'@';
    int opt;
    bool one_based = false;
    char* dfname = nullptr;
    while ((opt = getopt(argc, argv, "h1s:i:d:o:")) != -1) {
        switch (opt) {
        case 'i':
            if (freopen(optarg, "r", stdin) == NULL) {
                perror("freopen");
                exit(EXIT_FAILURE);
            }
            break;
        case 'o':
            if (freopen(optarg, "w", stdout) == NULL) {
                perror("freopen");
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            dfname = optarg;
            break;
        case '1':
            one_based = true;
            break;
        case 's':
            sepa = converter.from_bytes(optarg);
            if (sepa.length() != 1) {
                fprintf(stderr, "[E]: -s S: S must be a rune, it's \"%s\""
                        " instead\n", optarg);
                exit(EXIT_FAILURE);
            }
            s = sepa[0];
            break;
        case 'h':
            print_help();
            return 0;
        default:
            print_help();
            exit(EXIT_FAILURE);
        }
    }
    if (!dfname) {
        print_help();
        exit(EXIT_FAILURE);
    }
    std::vector<std::wstring> dict;
    if (one_based) {
        dict.emplace_back(L"");
    }
    {
        std::wifstream df(dfname);
        for (std::wstring s; std::getline(df, s);) {
            dict.push_back(std::move(s));
        }
    }
    wint_t c;
    while ((c = fgetwc(stdin)) != WEOF) {
        if (wchar_t(c) != s) {
            fputwc(c, stdout);
            continue;
        }
        size_t n = 0;
        bool esc = false;
        while ((c = fgetwc(stdin)) != WEOF && c >= '0' && c <= '9') {
            n = n*10 + c%48;
            esc = true;
        }
        if (esc && n < dict.size()) {
            fwprintf(stdout, L"%ls", dict[n].c_str());
        }
        if (c == WEOF) {
            break;
        }
        if (!esc && wchar_t(c) == s) {
            fputwc(c, stdout);
            continue;
        }
        ungetwc(c, stdin);
    }
    return 0;
}
