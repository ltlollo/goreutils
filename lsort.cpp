// gpp self

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

using namespace std;

int
main() {
    cin.sync_with_stdio(false);
    vector<string> vd{};
    string s;
    cin >> noskipws;
    while (getline(cin, s)) {
        vd.emplace_back(move(s));
    }
    sort(begin(vd), end(vd), [](const auto &f, const auto &s) noexcept {
        return f.size() > s.size();
    });
    for (const auto &it : vd) {
        cout << it << '\n';
    }
    return 0;
}

