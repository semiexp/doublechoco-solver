#include "Problem.h"

Problem::Problem(int height, int width)
    : height_(height), width_(width), color_(height * width, -1), num_(height * width, -1) {}

namespace {

int base36_to_int(char c) {
    if ('0' <= c && c <= '9')
        return c - '0';
    else
        return c - 'a' + 10;
}

bool is_base16(char c) { return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f'); }

bool is_base36(char c) { return ('0' <= c && c <= '9') || ('a' <= c && c <= 'z'); }

} // namespace

std::optional<Problem> Problem::ParseURL(const std::string& url) {
    std::string prefix("https://puzz.link/p?dbchoco/");
    if (url.size() < prefix.size() || url.substr(0, prefix.size()) != prefix) {
        return std::nullopt;
    }
    std::string body = url.substr(prefix.size());
    int height, width;
    {
        auto pos = body.find("/");
        if (pos == std::string::npos) {
            return std::nullopt;
        }
        width = std::stoi(body.substr(0, pos));
        body = body.substr(pos + 1);
    }
    {
        auto pos = body.find("/");
        if (pos == std::string::npos) {
            return std::nullopt;
        }
        height = std::stoi(body.substr(0, pos));
        body = body.substr(pos + 1);
    }
    int p = 0, idx = 0;
    Problem problem(height, width);
    while (idx < height * width) {
        if (!is_base36(body[p])) {
            return std::nullopt;
        }
        int n = base36_to_int(body[p++]);
        for (int i = 0; i < 5; ++i) {
            if (idx >= height * width)
                break;
            problem.setColor(idx / width, idx % width, (n >> (4 - i)) & 1);
            ++idx;
        }
    }
    idx = 0;
    while (idx < height * width) {
        if ('g' <= body[p] && body[p] <= 'z') {
            idx += base36_to_int(body[p]) - 15;
            ++p;
            continue;
        }
        int n;
        if (body[p] == '-') {
            if (!(is_base16(body[p + 1]) && is_base16(body[p + 2]))) {
                return std::nullopt;
            }
            n = (base36_to_int(body[p + 1]) << 4) | base36_to_int(body[p + 2]);
            p += 3;
        } else if (body[p] == '+') {
            if (!(is_base16(body[p + 1]) && is_base16(body[p + 2]) && is_base16(body[p + 3]))) {
                return std::nullopt;
            }
            n = (base36_to_int(body[p + 1]) << 8) | (base36_to_int(body[p + 2]) << 4) | base36_to_int(body[p + 3]);
            p += 4;
        } else {
            if (!is_base16(body[p])) {
                return std::nullopt;
            }
            n = base36_to_int(body[p]);
            ++p;
        }
        problem.setNum(idx / width, idx % width, n);
        ++idx;
    }
    return problem;
}
