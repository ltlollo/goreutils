// gpp self

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <codecvt>
#include <locale>
#include <experimental/string_view>
#include <string.h>

int main(int argc, char *argv[]) {
    auto print_usage = [&]() {
        std::wcerr << "Usage:\t" << argv[0] << L" N S\nScope:\tpresent"
            " text using N runes per line, using S to indicate line"
            " wrap.\n\tN<uint+>: runes per line (default: 79)\n\t"
            "S<str>: newline indicator (default ' ↵')" << std::endl;
    };
    std::locale::global(std::locale(""));
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    std::ostreambuf_iterator<wchar_t> wc{std::wcout};
    size_t max = (argc > 1) ? [&]() {
        size_t res;
        char* end;
        if ((res = std::strtoll(argv[1], &end, 10)) != 0 &&
            end == argv[1] + strlen(argv[1])) {
            // out of range is not an error
            return res;
        }
        std::wcerr << "[E]: " << converter.from_bytes(argv[1]) << "; 0/NaN\n";
        print_usage();
        exit(EXIT_FAILURE);
    }() : 79;
    std::wstring sepa = (argc > 2) ? converter.from_bytes(argv[2]) : L" ↵";
    if (max <= sepa.length() ) {
        std::copy(std::istreambuf_iterator<wchar_t>{std::wcin}, {}, wc);
        std::wcerr << "[E]: separator size must be greater than line size\n";
        exit(EXIT_FAILURE);
    }
    for (std::wstring s; std::getline(std::wcin, s);) {
        auto it = s.begin();
        while (size_t(distance(it, s.end())) > max) {
            auto rpos = std::experimental::wstring_view(&*it, max)
                .find_last_of(' ');
            if (rpos == std::wstring::npos) {
                std::copy(it, it+max-sepa.length(), wc);
                std::wcout << sepa << '\n';
                it += max-sepa.length();
            } else {
                *(it+rpos) = '\n';
                std::copy(it, it+rpos+1, wc);
                it += rpos+1;
            }
        }
        std::copy(it, s.end(), wc);
        std::wcout << std::endl;
    }
    return 0;
}

