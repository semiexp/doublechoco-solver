#include "evolmino/Problem.h"

namespace evolmino {

Problem::Problem(int height, int width) : height_(height), width_(width), cell_(height, width, Cell::kEmpty), arrow_id_(height, width, -1) {}

void Problem::AddArrow(Arrow&& arrow) {
    for (auto& [y, x] : arrow) {
        assert(arrow_id_.at(y, x) == -1);
        arrow_id_.at(y, x) = arrows_.size();
    }
    arrows_.push_back(std::move(arrow));
}

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
    std::string prefix("https://puzz.link/p?evolmino/");
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
    int p = 0;
    Problem problem(height, width);
    const int kPow3[] = {1, 3, 9};
    for (int i = 0; i < (height * width + 2) / 3; ++i) {
        if (p >= body.size() || !is_base36(body[p])) {
            return std::nullopt;
        }
        int n = base36_to_int(body[p++]);
        for (int j = 0; j < 3; ++j) {
            int v = n / kPow3[2 - j] % 3;
            if (v == 0) continue;
            if (i * 3 + j >= height * width) {
                return std::nullopt;
            }
            problem.SetCell((i * 3 + j) / width, (i * 3 + j) % width, v == 1 ? Cell::kBlack : Cell::kSquare);
        }
    }

    Grid<bool> up(height - 1, width, false), down(height - 1, width, false);
    Grid<bool> left(height, width - 1, false), right(height, width - 1, false);

    for (int t = 0; t < 2; ++t) {
        int idx = 0;
        int lim = (height - 1) * width + height * (width - 1);
        while (idx < lim) {
            if (p >= body.size() || !is_base36(body[p])) {
                return std::nullopt;
            }
            int n = base36_to_int(body[p++]);
            idx += n;
            if (n == 35) {
                continue;
            } else {
                if (idx >= lim) {
                    break;
                }
                if (idx >= height * (width - 1)) {
                    (t == 0 ? up : down).at((idx - height * (width - 1)) / width, (idx - height * (width - 1)) % width) = true;
                } else {
                    (t == 0 ? left : right).at(idx / (width - 1), idx % (width - 1)) = true;
                }
                idx += 1;
            }
        }
    }

    Grid<bool> visited(height, width, false);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (visited.at(y, x)) continue;

            bool has_in_edge = false;
            if (y > 0) has_in_edge |= down.at(y - 1, x);
            if (y < height - 1) has_in_edge |= up.at(y, x);
            if (x > 0) has_in_edge |= right.at(y, x - 1);
            if (x < width - 1) has_in_edge |= left.at(y, x);

            if (has_in_edge) continue;

            std::vector<std::pair<int, int>> arrow;
            int yp = y, xp = x;
            for (;;) {
                if (visited.at(yp, xp)) {
                    return std::nullopt;
                }
                visited.at(yp, xp) = true;
                arrow.push_back({yp, xp});

                int y2 = -1, x2 = -1;
                auto update_next = [&](int yd, int xd) {
                    if (y2 == -1) {
                        y2 = yd;
                        x2 = xd;
                        return true;
                    } else {
                        return false;
                    }
                };

                if (yp > 0 && up.at(yp - 1, xp) && !update_next(yp - 1, xp)) {
                    return std::nullopt;
                }
                if (yp < height - 1 && down.at(yp, xp) && !update_next(yp + 1, xp)) {
                    return std::nullopt;
                }
                if (xp > 0 && left.at(yp, xp - 1) && !update_next(yp, xp - 1)) {
                    return std::nullopt;
                }
                if (xp < width - 1 && right.at(yp, xp) && !update_next(yp, xp + 1)) {
                    return std::nullopt;
                }

                if (y2 == -1) {
                    break;
                } else {
                    yp = y2;
                    xp = x2;
                }
            }

            if (arrow.size() >= 2) {
                problem.AddArrow(std::move(arrow));
            }
        }
    }

    return problem;
}

}
