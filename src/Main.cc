#include "Problem.h"
#include "Solver.h"

#include <cstdlib>
#include <string>

int base36_to_int(char c) {
    if ('0' <= c && c <= '9')
        return c - '0';
    else
        return c - 'a' + 10;
}

Problem ParseURL(const std::string& url) {
    std::string prefix("https://puzz.link/p?dbchoco/");
    if (url.size() < prefix.size() || url.substr(0, prefix.size()) != prefix) {
        abort();
    }
    std::string body = url.substr(prefix.size());
    int height, width;
    {
        auto pos = body.find("/");
        if (pos == std::string::npos)
            abort();
        width = std::stoi(body.substr(0, pos));
        body = body.substr(pos + 1);
    }
    {
        auto pos = body.find("/");
        if (pos == std::string::npos)
            abort();
        height = std::stoi(body.substr(0, pos));
        body = body.substr(pos + 1);
    }
    int p = 0, idx = 0;
    Problem problem(height, width);
    while (idx < height * width) {
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
            n = (base36_to_int(body[p + 1]) << 4) | base36_to_int(body[p + 2]);
            p += 3;
        } else if (body[p] == '+') {
            n = (base36_to_int(body[p + 1]) << 8) | (base36_to_int(body[p + 2]) << 4) | base36_to_int(body[p + 3]);
            p += 4;
        } else {
            n = base36_to_int(body[p]);
            ++p;
        }
        problem.setNum(idx / width, idx % width, n);
        ++idx;
    }
    return problem;
}

int main(int argc, char** argv) {
    Problem problem = ParseURL(argv[1]);
    int height = problem.height();
    int width = problem.width();

    std::optional<DoublechocoAnswer> ans = FindAnswer(problem);
    assert(ans.has_value());

    for (int y = -1; y < height * 2; ++y) {
        for (int x = -1; x < width * 2; ++x) {
            if ((y & 1) == 0 && (x & 1) == 0) {
                printf(" ");
            } else if ((y & 1) == 1 && (x & 1) == 1) {
                printf("+");
            } else if ((y & 1) == 1 && (x & 1) == 0) {
                if (y == -1 || y == height * 2 - 1) {
                    printf("-");
                    continue;
                }
                switch (ans->vertical[y / 2][x / 2]) {
                case DoublechocoAnswer::Border::kUndecided:
                    printf("?");
                    break;
                case DoublechocoAnswer::Border::kWall:
                    printf("-");
                    break;
                case DoublechocoAnswer::Border::kConnected:
                    printf(" ");
                    break;
                }
            } else {
                if (x == -1 || x == width * 2 - 1) {
                    printf("|");
                    continue;
                }
                switch (ans->horizontal[y / 2][x / 2]) {
                case DoublechocoAnswer::Border::kUndecided:
                    printf("?");
                    break;
                case DoublechocoAnswer::Border::kWall:
                    printf("|");
                    break;
                case DoublechocoAnswer::Border::kConnected:
                    printf(" ");
                    break;
                }
            }
        }
        printf("\n");
    }
    printf("\n");

    return 0;
}
