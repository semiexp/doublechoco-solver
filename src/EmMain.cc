#include <emscripten/bind.h>

using namespace emscripten;

#include "doublechoco/Problem.h"
#include "doublechoco/Solver.h"
#include "evolmino/Problem.h"
#include "evolmino/Solver.h"

#include <sstream>
#include <string>

std::string solve_doublechoco(const std::string& url) {
    using namespace doublechoco;

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

std::string solve_evolmino(const std::string& url) {
    using namespace evolmino;

    std::optional<Problem> problem_opt = Problem::ParseURL(url);
    if (!problem_opt) {
        return "{\"description\":\"invalid url\"}";
    }
    Problem problem = *problem_opt;

    std::optional<EvolminoAnswer> ans = Solve(problem);
    if (!ans) {
        return "{\"description\":\"no answer\"}";
    }

    int height = problem.height();
    int width = problem.width();

    std::ostringstream oss;
    oss << "{\"description\":{\"kind\":\"grid\",\"height\":" << height << ",\"width\":" << width
        << ",\"defaultStyle\":\"grid\",\"data\":[";
    bool is_first = true;
    for (int i = 0; i < problem.NumArrows(); ++i) {
        const auto& arrow = problem.GetArrow(i);
        for (int j = 1; j < arrow.size(); ++j) {
            if (!is_first) {
                oss << ",";
            } else {
                is_first = false;
            }
            int y = arrow[j - 1].first + arrow[j].first + 1;
            int x = arrow[j - 1].second + arrow[j].second + 1;
            oss << "{\"y\":" << y << ",\"x\":" << x << ",\"color\":\"black\",\"item\":\"line\"}";
        }
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            switch (problem.cell(y, x)) {
                case Problem::Cell::kEmpty:
                    break;
                case Problem::Cell::kBlack:
                    if (!is_first) {
                        oss << ",";
                    } else {
                        is_first = false;
                    }
                    oss << "{\"y\":" << y * 2 + 1 << ",\"x\":" << x * 2 + 1 << ",\"color\":\"black\",\"item\":\"fill\"}";
                    continue;
                case Problem::Cell::kSquare:
                    if (!is_first) {
                        oss << ",";
                    } else {
                        is_first = false;
                    }
                    oss << "{\"y\":" << y * 2 + 1 << ",\"x\":" << x * 2 + 1 << ",\"color\":\"black\",\"item\":\"square\"}";
                    continue;
            }

            if (ans->at(y, x) == EvolminoAnswerCell::kSquare) {
                if (!is_first) {
                    oss << ",";
                } else {
                    is_first = false;
                }
                oss << "{\"y\":" << y * 2 + 1 << ",\"x\":" << x * 2 + 1 << ",\"color\":\"green\",\"item\":\"square\"}";
            } else if (ans->at(y, x) == EvolminoAnswerCell::kEmpty) {
                if (!is_first) {
                    oss << ",";
                } else {
                    is_first = false;
                }
                oss << "{\"y\":" << y * 2 + 1 << ",\"x\":" << x * 2 + 1 << ",\"color\":\"green\",\"item\":\"dot\"}";
            }
        }
    }

    oss << "]}}";

    return oss.str();
}

std::string solve(const std::string& url) {
    std::string dbchoco_prefix("https://puzz.link/p?dbchoco/");
    if (url.size() >= dbchoco_prefix.size() && url.substr(0, dbchoco_prefix.size()) == dbchoco_prefix) {
        return solve_doublechoco(url);
    }

    std::string evolmino_prefix("https://puzz.link/p?evolmino/");
    if (url.size() >= evolmino_prefix.size() && url.substr(0, evolmino_prefix.size()) == evolmino_prefix) {
        return solve_evolmino(url);
    }

    return "{\"description\":\"invalid url\"}";
}

EMSCRIPTEN_BINDINGS(doublechoco_solver) { function("solve", &solve); }
