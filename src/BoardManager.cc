#include "BoardManager.h"

#include <cassert>

BoardManager::BoardManager(int height, int width, Glucose::Var origin)
    : height_(height), width_(width), origin_(origin), horizontal_(height * (width - 1), Border::kUndecided),
      vertical_((height - 1) * width, Border::kUndecided) {}

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
