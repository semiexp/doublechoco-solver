#include <emscripten/bind.h>

using namespace emscripten;

#include "Problem.h"
#include "Solver.h"

#include <sstream>
#include <string>

std::string solve(const std::string& url) {
    std::optional<Problem> problem_opt = Problem::ParseURL(url);
    if (!problem_opt) {
        return "{\"description\":\"invalid url\"}";
    }
    Problem problem = *problem_opt;

    std::optional<DoublechocoAnswer> ans = Solve(problem);
    if (!ans) {
        return "{\"description\":\"no answer\"}";
    }

    int height = problem.height();
    int width = problem.width();

    std::ostringstream oss;
    oss << "{\"description\":{\"kind\":\"grid\",\"height\":" << height << ",\"width\":" << width
        << ",\"defaultStyle\":\"outer_grid\",\"data\":[";
    bool is_first = true;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (problem.color(y, x) == 1) {
                if (!is_first) {
                    oss << ",";
                } else {
                    is_first = false;
                }
                oss << "{\"y\":" << y * 2 + 1 << ",\"x\":" << x * 2 + 1 << ",\"color\":\"#eeeeee\",\"item\":\"fill\"}";
            }
            if (problem.num(y, x) > 0) {
                if (!is_first) {
                    oss << ",";
                } else {
                    is_first = false;
                }
                int n = problem.num(y, x);
                oss << "{\"y\":" << y * 2 + 1 << ",\"x\":" << x * 2 + 1 << ",\"color\":\"black\","
                    << "\"item\":{\"kind\":\"text\",\"data\":\"" << n << "\"}}";
            }
        }
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (y < height - 1) {
                if (ans->vertical[y][x] != DoublechocoAnswer::Border::kWall) {
                    oss << ",{\"y\":" << y * 2 + 2 << ",\"x\":" << x * 2 + 1
                        << ",\"color\":\"#cccccc\",\"item\":\"wall\"}";
                }
                if (ans->vertical[y][x] != DoublechocoAnswer::Border::kUndecided) {
                    const char* kind = ans->vertical[y][x] == DoublechocoAnswer::Border::kWall ? "boldWall" : "cross";
                    oss << ",{\"y\":" << y * 2 + 2 << ",\"x\":" << x * 2 + 1 << ",\"color\":\"green\",\"item\":\""
                        << kind << "\"}";
                }
            }
            if (x < width - 1) {
                if (ans->horizontal[y][x] != DoublechocoAnswer::Border::kWall) {
                    oss << ",{\"y\":" << y * 2 + 1 << ",\"x\":" << x * 2 + 2
                        << ",\"color\":\"#cccccc\",\"item\":\"wall\"}";
                }
                if (ans->horizontal[y][x] != DoublechocoAnswer::Border::kUndecided) {
                    const char* kind = ans->horizontal[y][x] == DoublechocoAnswer::Border::kWall ? "boldWall" : "cross";
                    oss << ",{\"y\":" << y * 2 + 1 << ",\"x\":" << x * 2 + 2 << ",\"color\":\"green\",\"item\":\""
                        << kind << "\"}";
                }
            }
        }
    }
    oss << "]}}";

    return oss.str();
}

EMSCRIPTEN_BINDINGS(doublechoco_solver) { function("solve", &solve); }
