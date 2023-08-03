#pragma once

#include <cassert>
#include <optional>
#include <string>
#include <vector>

#include "Grid.h"

namespace evolmino {

using Arrow = std::vector<std::pair<int, int>>;

class Problem {
public:
    enum Cell {
        kEmpty,
        kBlack,
        kSquare,
    };

    Problem(int height, int width);
    Problem(const Problem&) = default;

    int height() const { return height_; }
    int width() const { return width_; }

    Cell cell(int y, int x) const { return cell_.at(y, x); }
    void SetCell(int y, int x, Cell c) { cell_.at(y, x) = c; }

    int NumArrows() const { return arrows_.size(); }
    const Arrow& GetArrow(int idx) const { return arrows_[idx]; }
    void AddArrow(Arrow&& arrow);
    int GetArrowId(int y, int x) const { return arrow_id_.at(y, x); }

    static std::optional<Problem> ParseURL(const std::string& url);

private:
    int height_, width_;
    Grid<Cell> cell_;
    Grid<int> arrow_id_;
    std::vector<Arrow> arrows_;
};

}
