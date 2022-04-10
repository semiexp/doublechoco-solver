#pragma once

#include <cassert>
#include <optional>
#include <string>
#include <vector>

class Problem {
public:
    Problem(int height, int width);
    Problem(const Problem&) = default;

    int height() const { return height_; }
    int width() const { return width_; }

    int color(int y, int x) const { return color_[index(y, x)]; }
    void setColor(int y, int x, int c) { color_[index(y, x)] = c; }

    int num(int y, int x) const { return num_[index(y, x)]; }
    void setNum(int y, int x, int n) { num_[index(y, x)] = n; }

    static std::optional<Problem> ParseURL(const std::string& url);

private:
    int height_, width_;
    std::vector<int> color_, num_;

    int index(int y, int x) const {
        assert(0 <= y && y < height_ && 0 <= x && x < width_);
        return y * width_ + x;
    }
};
