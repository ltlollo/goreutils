// gpp self

#include <vector>
#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>

template<typename L, typename T, typename U>
void rparse_next_vec(const L& line, std::string& out, T& it, T& sep,
                     const U& end, bool top=true) {
    std::string str, num;
    if (!top) {
        out += ", ";
    }
    str = std::string(it, sep);
    out += "{\"" + move(str) + "\"";
    if (++sep == end) {
        std::cerr << "[E]: no cannot parse number after \\ in\n\t"
                  << *line << std::endl;
        exit(EXIT_FAILURE);
    }
    it = sep;
    while (sep != end && *sep >= '0' && *sep <= '9') {
        ++sep;
    }
    num = std::string(it, sep);
    if (num.empty()) {
        std::cerr << "[E]: no cannot parse number after \\ in\n\t"
                  << *line << std::endl;
        exit(EXIT_FAILURE);
    }
    if (stoul(num)) {
        out += ", " + move(num);
    }
    out += "}";
    it = sep;
    if ((sep = find(it, end, '\\')) != end) {
        rparse_next_vec(line, out, it, sep, end, false);
    }
}
template<typename L, typename T, typename U>
void parse_vecnum_rest(const L& line, std::string& out, T& it, const U& end) {
    std::string rx, rest;
    auto it_cp = it;
    out += "{";
    if ((it = find(it, end, '\\')) == end) {
        rx = std::string(it_cp, it);
        out += "{\"" + move(rx) + "\"}";
    } else {
        rparse_next_vec(line, out, it_cp, it, end);
    }
    out += "}";
    rest = std::string(it, end);
    if (!rest.empty()) {
        out += ", \"" + move(rest) + "\"";
    }
}

/*
 * Grammar:
 *  Separator Nl Rules
 * Separator:
 *  Ignore Chars Nl Ignore
 * Rules:
 *  (Ignore Rule Rules Inore) || Nil
 * Rule:
 *  Rx Spaces Sepa StrNum
 * Ignore: ("#" (Chars || Nil) Nl) || Nil
 * Nl: "\n"
 * Nil: ""
 */

int main(int argc, char *argv[]) {
    auto print_help = [&]() {
        std::cerr << "Usage:\t" << argv[0] << " grammar"
            "\n\tgrammar<file>: file containing the regex"
            "\nScope:\tgenerates the Rules struct to be used by soppe"
             << std::endl;
    };
    if (argc-1 != 1) {
        print_help();
        return 1;
    }
    auto f = std::ifstream(argv[1]);
    std::vector<std::string> lines;
    for (std::string line; getline(f, line);) {
        if (line.size() && line[0] != '#') {
            lines.push_back(move(line));
        }
    }
    if (lines.empty()) {
        std::cerr << "[E]: must contain the separator" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::string& sepa = lines[0];
    std::string out;
    out += "Rule rules[] = {\n";
    for (auto line = lines.begin()+1; line != lines.end(); ++line) {
        auto end = line->end();
        auto it = search(line->begin(), end, sepa.begin(), sepa.end());
        if (it == line->end() || it == line->begin()) {
            std::cerr << "[E]: line does not contain a separator\n\t"
                 << *line << std::endl;
            exit(EXIT_FAILURE);
        }
        auto it_cp = it;
        while (*(it_cp-1) == ' ') {
            --it_cp;
        }
        out += "    {\"" + std::string(line->begin(), it_cp) + "\"_r, ";
        it += sepa.size();
        while (it != line->end() && *it == ' ') {
            ++it;
        }
        parse_vecnum_rest(line, out, it, end);
        out += (line+1 != lines.end() ? "},\n" : "}\n");
    }
    out += "};\n";
    std::cout << out;
    return 0;
}

