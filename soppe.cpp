// sacca $HOME/.soppe_conf > soppe_conf.h && gpp self

#include <vector>
#include <stdexcept>
#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <unistd.h>

bool needs_escape(const char c) noexcept {
    return c == '(' || c == ')' || c == ' ' || c == '\'' || c == '"'
        || c == '&';
}

auto escape(const std::string& s) {
    std::string res;
    res.reserve(s.size());
    for (auto it = s.begin(); it != s.end(); ++it) {
        if (needs_escape(*it)) {
            res += '\\';
        }
        res += *it;
    }
    return res;
}

auto escape(int argc, char* argv[], int start=1) {
    std::string res;
    if (start >= argc) return res;
    for (int i = start; i < argc; ++i) {
        for(char* it = argv[i]; *it != '\0'; ++it) {
            if (needs_escape(*it)) {
                res += '\\';
            }
            res += *it;
        }
        if (i < argc-1) res += ' ';
    }
    return res;
}

static auto operator""_r(const char* str, std::size_t) {
    return std::regex(str, std::regex::optimize);
}

struct StrNum{ std::string str{}; unsigned num{}; };
struct Rule{std::regex r; std::vector<StrNum> strnunms; std::string rest{}; };

#include "soppe_conf.h"

void call(const std::string& str) {
    std::size_t i = 0;
    for (const auto& rule: rules) {
        std::smatch what;
        if (regex_match(str, what, rule.r) &&
            what.size() == rule.strnunms.size() + 1) {
            std::string cmd;
            for (const auto& strnum: rule.strnunms) {
                cmd += strnum.str;
                if (strnum.num+1 >= what.size()) {
                    std::cerr << "[W]: position outside range in rule number "
                              << i;
                }
                cmd += what[strnum.num+1];
            }
            cmd += rule.rest;
            execl("/bin/sh", "/bin/sh", "-c", cmd.c_str(), nullptr);
        }
        ++i;
    }
}

int main(int argc, char *argv[]) {
    call(escape(argc, argv));
    return 1;
}

