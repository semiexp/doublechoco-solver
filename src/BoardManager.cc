#include "BoardManager.h"

#include <cassert>
#include <queue>

GroupInfo::GroupInfo(Grid<int>&& group_id) : group_id_(group_id) {
    int max_group_id = 0;
    for (int y = 0; y < group_id_.height(); ++y) {
        for (int x = 0; x < group_id_.width(); ++x) {
            max_group_id = std::max(max_group_id, group_id_.at(y, x));
        }
    }

    groups_offset_ = std::vector<int>(max_group_id + 2, 0);
    for (int y = 0; y < group_id_.height(); ++y) {
        for (int x = 0; x < group_id_.width(); ++x) {
            ++groups_offset_[group_id_.at(y, x) + 1];
        }
    }
    for (int i = 1; i < groups_offset_.size(); ++i) {
        groups_offset_[i] += groups_offset_[i - 1];
    }
    std::vector<int> next_pos = groups_offset_;
    groups_raw_ = std::vector<std::pair<int, int>>(group_id_.height() * group_id_.width());
    for (int y = 0; y < group_id_.height(); ++y) {
        for (int x = 0; x < group_id_.width(); ++x) {
            groups_raw_[next_pos[group_id_.at(y, x)]++] = std::make_pair(y, x);
        }
    }
}

BoardManager::BoardManager(const Problem& problem, Glucose::Var origin)
    : height_(problem.height()), width_(problem.width()), problem_(problem), origin_(origin),
      horizontal_(problem.height() * (problem.width() - 1), Border::kUndecided),
      vertical_((problem.height() - 1) * problem.width(), Border::kUndecided) {}

BoardManager::Border BoardManager::horizontal(int y, int x) const {
    assert(0 <= y && y < height_ && 0 <= x && x < width_ - 1);
    return horizontal_[y * (width_ - 1) + x];
}

BoardManager::Border BoardManager::vertical(int y, int x) const {
    assert(0 <= y && y < height_ - 1 && 0 <= x && x < width_);
    return vertical_[y * width_ + x];
}

Glucose::Var BoardManager::HorizontalVar(int y, int x) const {
    assert(0 <= y && y < height_ && 0 <= x && x < width_ - 1);
    return origin_ + y * (width_ - 1) + x;
}

Glucose::Var BoardManager::VerticalVar(int y, int x) const {
    assert(0 <= y && y < height_ - 1 && 0 <= x && x < width_);
    return origin_ + height_ * (width_ - 1) + y * width_ + x;
}

void BoardManager::Decide(Glucose::Lit lit) {
    Glucose::Var v = Glucose::var(lit);
    int ofs = v - origin_;
    assert(0 <= ofs && ofs < height_ * (width_ - 1) + (height_ - 1) * width_);
    bool sign = Glucose::sign(lit);
    Border new_value = sign ? Border::kConnected : Border::kWall;

    if (ofs < height_ * (width_ - 1)) {
        assert(horizontal_[ofs] == Border::kUndecided);
        horizontal_[ofs] = new_value;
    } else {
        ofs -= height_ * (width_ - 1);
        assert(vertical_[ofs] == Border::kUndecided);
        vertical_[ofs] = new_value;
    }
    decisions_.push_back(lit);
}

void BoardManager::Undo(Glucose::Lit lit) {
    assert(!decisions_.empty());
    assert(decisions_.back() == lit);

    Glucose::Var v = Glucose::var(lit);
    int ofs = v - origin_;
    assert(0 <= ofs && ofs < height_ * (width_ - 1) + (height_ - 1) * width_);

    if (ofs < height_ * (width_ - 1)) {
        horizontal_[ofs] = Border::kUndecided;
    } else {
        ofs -= height_ * (width_ - 1);
        vertical_[ofs] = Border::kUndecided;
    }
    decisions_.pop_back();
}

std::vector<Glucose::Var> BoardManager::RelatedVariables() const {
    std::vector<Glucose::Var> ret;
    int n_vars = height_ * (width_ - 1) + (height_ - 1) * width_;
    for (int i = 0; i < n_vars; ++i) {
        ret.push_back(origin_ + i);
    }
    return ret;
}

std::vector<Glucose::Lit> BoardManager::ReasonForBlock(const BoardInfo& info, int block_id) const {
    std::vector<Glucose::Lit> ret;
    for (auto [y, x] : info.blocks.group(block_id)) {
        if (y < height_ - 1 && info.blocks.group_id(y + 1, x) == block_id && vertical(y, x) == Border::kConnected) {
            ret.push_back(Glucose::mkLit(VerticalVar(y, x), true));
        }
        if (x < width_ - 1 && info.blocks.group_id(y, x + 1) == block_id && horizontal(y, x) == Border::kConnected) {
            ret.push_back(Glucose::mkLit(HorizontalVar(y, x), true));
        }
    }
    return ret;
}

std::vector<Glucose::Lit> BoardManager::ReasonForUnit(const BoardInfo& info, int block_id) const {
    std::vector<Glucose::Lit> ret;
    for (auto [y, x] : info.units.group(block_id)) {
        if (y < height_ - 1 && info.units.group_id(y + 1, x) == block_id && vertical(y, x) == Border::kConnected) {
            ret.push_back(Glucose::mkLit(VerticalVar(y, x), true));
        }
        if (x < width_ - 1 && info.units.group_id(y, x + 1) == block_id && horizontal(y, x) == Border::kConnected) {
            ret.push_back(Glucose::mkLit(HorizontalVar(y, x), true));
        }
    }
    return ret;
}

std::vector<Glucose::Lit> BoardManager::ReasonForPotentialUnitBoundary(const BoardInfo& info,
                                                                       int potential_unit_id) const {
    std::vector<Glucose::Lit> ret;
    for (auto [y, x] : info.potential_units.group(potential_unit_id)) {
        if (y > 0 && info.potential_units.group_id(y - 1, x) != potential_unit_id &&
            problem_.color(y, x) == problem_.color(y - 1, x) && vertical(y - 1, x) == Border::kWall) {
            ret.push_back(Glucose::mkLit(VerticalVar(y - 1, x)));
        }
        if (y < height_ - 1 && info.potential_units.group_id(y + 1, x) != potential_unit_id &&
            problem_.color(y, x) == problem_.color(y + 1, x) && vertical(y, x) == Border::kWall) {
            ret.push_back(Glucose::mkLit(VerticalVar(y, x)));
        }
        if (x > 0 && info.potential_units.group_id(y, x - 1) != potential_unit_id &&
            problem_.color(y, x) == problem_.color(y, x - 1) && horizontal(y, x - 1) == Border::kWall) {
            ret.push_back(Glucose::mkLit(HorizontalVar(y, x - 1)));
        }
        if (x < width_ - 1 && info.potential_units.group_id(y, x + 1) != potential_unit_id &&
            problem_.color(y, x) == problem_.color(y, x + 1) && horizontal(y, x) == Border::kWall) {
            ret.push_back(Glucose::mkLit(HorizontalVar(y, x)));
        }
    }
    return ret;
}

std::vector<Glucose::Lit> BoardManager::ReasonForPath(int ya, int xa, int yb, int xb) const {
    Grid<std::pair<int, int>> from(height_, width_, std::make_pair(-1, -1));
    from.at(ya, xa) = std::make_pair(-2, -2);

    std::queue<std::pair<int, int>> qu;
    qu.push({ya, xa});
    while (!qu.empty()) {
        auto [y, x] = qu.front();
        qu.pop();
        if (y == yb && x == xb) {
            break;
        }

        if (y > 0 && vertical(y - 1, x) == Border::kConnected && from.at(y - 1, x).first == -1) {
            from.at(y - 1, x) = std::make_pair(y, x);
            qu.push({y - 1, x});
        }
        if (y < height_ - 1 && vertical(y, x) == Border::kConnected && from.at(y + 1, x).first == -1) {
            from.at(y + 1, x) = std::make_pair(y, x);
            qu.push({y + 1, x});
        }
        if (x > 0 && horizontal(y, x - 1) == Border::kConnected && from.at(y, x - 1).first == -1) {
            from.at(y, x - 1) = std::make_pair(y, x);
            qu.push({y, x - 1});
        }
        if (x < width_ - 1 && horizontal(y, x) == Border::kConnected && from.at(y, x + 1).first == -1) {
            from.at(y, x + 1) = std::make_pair(y, x);
            qu.push({y, x + 1});
        }
    }

    assert(from.at(yb, xb).first != -1);

    std::vector<Glucose::Lit> ret;
    int y = yb;
    int x = xb;
    while (!(y == ya && x == xa)) {
        auto [y_from, x_from] = from.at(y, x);

        if (y == y_from) {
            ret.push_back(Glucose::mkLit(HorizontalVar(y, std::min(x, x_from)), true));
        } else {
            ret.push_back(Glucose::mkLit(VerticalVar(std::min(y, y_from), x), true));
        }
        y = y_from;
        x = x_from;
    }
    return ret;
}

void BoardManager::calcReasonSimple(Glucose::Lit p, Glucose::Lit extra, Glucose::vec<Glucose::Lit>& out_reason) {
    for (const Glucose::Lit lit : decisions_) {
        out_reason.push(lit);
    }
    if (extra != Glucose::lit_Undef) {
        assert(p == Glucose::lit_Undef);
        out_reason.push(extra);
    }
}

Glucose::Var BoardManager::AllocateVariables(Glucose::Solver& solver, int height, int width) {
    int n_vars = height * (width - 1) + (height - 1) * width;
    Glucose::Var head = solver.newVar();
    for (int i = 1; i < n_vars; ++i) {
        solver.newVar();
    }
    return head;
}

namespace {

void ComputeConnectedComponentsSearch(const BoardManager& board, bool ignore_color, bool is_potential,
                                      Grid<int>& group_id, int y, int x, int id) {
    if (group_id.at(y, x) != -1) {
        return;
    }
    group_id.at(y, x) = id;

    auto maybe_traverse = [&](int y2, int x2, BoardManager::Border border) {
        if (!ignore_color && board.problem().color(y2, x2) != board.problem().color(y, x)) {
            return;
        }
        if (border == BoardManager::Border::kConnected ||
            (is_potential && border == BoardManager::Border::kUndecided)) {
            ComputeConnectedComponentsSearch(board, ignore_color, is_potential, group_id, y2, x2, id);
        }
    };

    if (y > 0) {
        maybe_traverse(y - 1, x, board.vertical(y - 1, x));
    }
    if (y < board.height() - 1) {
        maybe_traverse(y + 1, x, board.vertical(y, x));
    }
    if (x > 0) {
        maybe_traverse(y, x - 1, board.horizontal(y, x - 1));
    }
    if (x < board.width() - 1) {
        maybe_traverse(y, x + 1, board.horizontal(y, x));
    }
}

Grid<int> ComputeConnectedComponents(const BoardManager& board, bool ignore_color, bool is_potential) {
    Grid<int> group_id(board.height(), board.width(), -1);
    int id_last = 0;
    for (int y = 0; y < board.height(); ++y) {
        for (int x = 0; x < board.width(); ++x) {
            if (group_id.at(y, x) == -1) {
                ComputeConnectedComponentsSearch(board, ignore_color, is_potential, group_id, y, x, id_last++);
            }
        }
    }
    return group_id;
}

} // namespace

BoardInfo BoardManager::ComputeBoardInfo() const {
    return BoardInfo{
        ComputeConnectedComponents(*this, false, false),
        ComputeConnectedComponents(*this, true, false),
        ComputeConnectedComponents(*this, false, true),
    };
}

void BoardManager::Dump() const {
    for (int y = 0; y <= (height_ - 1) * 2; ++y) {
        for (int x = 0; x <= (width_ - 1) * 2; ++x) {
            if (y % 2 == 0 && x % 2 == 0) {
                printf(" ");
            } else if (y % 2 == 1 && x % 2 == 1) {
                printf("+");
            } else if (y % 2 == 1 && x % 2 == 0) {
                switch (vertical(y / 2, x / 2)) {
                case Border::kUndecided:
                    printf("?");
                    break;
                case Border::kWall:
                    printf("-");
                    break;
                case Border::kConnected:
                    printf(" ");
                    break;
                }
            } else {
                switch (horizontal(y / 2, x / 2)) {
                case Border::kUndecided:
                    printf("?");
                    break;
                case Border::kWall:
                    printf("|");
                    break;
                case Border::kConnected:
                    printf(" ");
                    break;
                }
            }
        }
        printf("\n");
    }
    printf("\n");
}
