//
// Created by mateusz on 19.04.2021.
//
#include <bits/stdc++.h>

#define _GNU_SOURCE         /* See feature_test_macros(7) */

#include <stdlib.h>

namespace {
    const std::string method = "(GET)|(HEAD)";
    using vs_t = std::vector<std::string>;
    const std::string HTTP_VERSION = "HTTP/1.1";
    const std::string request_target = "(\\[a-zA-Z0-9.-]+)";
    const std::string CRLF = "\r\n";
    static const std::regex request_line_regex(method + " " + request_target + " " + HTTP_VERSION);
}

std::filesystem::path normalized_trimed(const std::filesystem::path &p) {
    auto r = p.lexically_normal();
    printf("P = %s, Lexically normal P = %s\n", p.c_str(), r.c_str());
    if (r.has_filename()) return r;
    return r.parent_path();
}

bool is_subpath_of(const std::filesystem::path &base, const std::filesystem::path &sub) {
    std::cout << "PRZED B = " << base << std::endl;
    auto b = normalized_trimed(base);
    std::cout << "PO B = " << b << std::endl;
    std::cout << "PRZED A = " << sub << std::endl;
    auto s = normalized_trimed(sub).parent_path();
    std::cout << "PO A = " << s << std::endl;
    auto m = std::mismatch(b.begin(), b.end(),
                           s.begin(), s.end());
    return m.first == b.end();
}


int main() {
    std::string a, b;
    std::cin >> a >> b;
    std::filesystem::path p2(std::string(canonicalize_file_name(a.c_str())) + "/" + b);
    std::filesystem::path p1(std::filesystem::path(std::string(canonicalize_file_name(a.c_str())) + "/"));
    char *xd = canonicalize_file_name(p2.c_str());
    printf("XD = %s\n", xd);
    std::cout << "Istnieje(p1) = " << std::to_string(std::filesystem::exists(p1)) << std::endl;
    std::cout << "Istnieje(p2) = " << std::to_string(std::filesystem::exists(p2)) << std::endl;
    std::cout << "p1.tostring() = " << p1 << std::endl;
    std::cout << "p2.tostring() = " << p2 << std::endl;
    std::cout << is_subpath_of(p1, p2);
}
