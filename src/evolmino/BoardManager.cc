#include "evolmino/BoardManager.h"

#include <cassert>

namespace evolmino {

BoardManager::BoardManager(const Problem& problem, Glucose::Var origin)
    : height_(problem.height()), width_(problem.width()), problem_(problem), origin_(origin),
      cells_(problem.height() * problem.width(), Cell::kUndecided) {}

BoardManager::Cell BoardManager::cell(int y, int x) const {
    assert(0 <= y && y < height_ && 0 <= x && x < width_);
    return cells_[y * width_ + x];
}

Glucose::Var BoardManager::CellVar(int y, int x) const {
    assert(0 <= y && y < height_ && 0 <= x && x < width_);
    return origin_ + y * width_ + x;
}

void BoardManager::Decide(Glucose::Lit lit) {
    Glucose::Var v = Glucose::var(lit);
    int ofs = v - origin_;
    assert(0 <= ofs && ofs < height_ * width_);
    bool sign = Glucose::sign(lit);
    Cell new_value = sign ? Cell::kEmpty : Cell::kSquare;

    assert(cells_[ofs] == Cell::kUndecided);
    cells_[ofs] = new_value;

    decisions_.push_back(lit);
}

void BoardManager::Undo(Glucose::Lit lit) {
    assert(!decisions_.empty());
    assert(decisions_.back() == lit);

    Glucose::Var v = Glucose::var(lit);
    int ofs = v - origin_;
    assert(0 <= ofs && ofs < height_ * (width_ - 1) + (height_ - 1) * width_);
    cells_[ofs] = Cell::kUndecided;

    decisions_.pop_back();
}

std::vector<Glucose::Var> BoardManager::RelatedVariables() const {
    std::vector<Glucose::Var> ret;
    int n_vars = height_ * width_;
    for (int i = 0; i < n_vars; ++i) {
        ret.push_back(origin_ + i);
    }
    return ret;
}

std::vector<Glucose::Lit> BoardManager::ReasonNaive() const {
    return decisions_;
}

Glucose::Var BoardManager::AllocateVariables(Glucose::Solver& solver, int height, int width) {
    int n_vars = height * width;
    Glucose::Var head = solver.newVar();
    for (int i = 1; i < n_vars; ++i) {
        solver.newVar();
    }
    return head;
}

namespace {

void ComputeConnectedComponentsSearch(const BoardManager& board, bool is_potential, Grid<int>& group_id, int y, int x, int id) {
    if (group_id.at(y, x) != -1) {
        return;
    }
    if (is_potential) {
        if (board.cell(y, x) == BoardManager::Cell::kEmpty) return;
    } else {
        if (board.cell(y, x) != BoardManager::Cell::kSquare) return;
    }

    group_id.at(y, x) = id;

    if (y > 0) {
        ComputeConnectedComponentsSearch(board, is_potential, group_id, y - 1, x, id);
    }
    if (y < board.height() - 1) {
        ComputeConnectedComponentsSearch(board, is_potential, group_id, y + 1, x, id);
    }
    if (x > 0) {
        ComputeConnectedComponentsSearch(board, is_potential, group_id, y, x - 1, id);
    }
    if (x < board.width() - 1) {
        ComputeConnectedComponentsSearch(board, is_potential, group_id, y, x + 1, id);
    }
}

Grid<int> ComputeConnectedComponents(const BoardManager& board, bool is_potential) {
    Grid<int> group_id(board.height(), board.width(), -1);
    int id_last = 0;

    for (int y = 0; y < board.height(); ++y) {
        for (int x = 0; x < board.width(); ++x) {
            if (group_id.at(y, x) == -1 && (is_potential || board.cell(y, x) == BoardManager::Cell::kSquare)) {
                if (is_potential) {
                    if (board.cell(y, x) == BoardManager::Cell::kEmpty) continue;
                } else {
                    if (board.cell(y, x) != BoardManager::Cell::kSquare) continue;
                }
                ComputeConnectedComponentsSearch(board, is_potential, group_id, y, x, id_last++);
            }
        }
    }

    return group_id;
}

}

BoardInfoSimple BoardManager::ComputeBoardInfoSimple() const {
    return BoardInfoSimple {
        ComputeConnectedComponents(*this, false),
        ComputeConnectedComponents(*this, true),
    };
}

namespace {

const int kFourNeighborY[] = {-1, 0, 1, 0};
const int kFourNeighborX[] = {0, -1, 0, 1};

void TraverseFloatingComponents(Grid<std::pair<BoardInfoDetailed::CellKind, int>>& cell_info, int y, int x, int id) {
    if (cell_info.at(y, x).second != -2) return;

    cell_info.at(y, x) = std::make_pair(BoardInfoDetailed::CellKind::kFloating, id);

    if (y > 0) {
        TraverseFloatingComponents(cell_info, y - 1, x, id);
    }
    if (y < cell_info.height() - 1) {
        TraverseFloatingComponents(cell_info, y + 1, x, id);
    }
    if (x > 0) {
        TraverseFloatingComponents(cell_info, y, x - 1, id);
    }
    if (x < cell_info.width() - 1) {
        TraverseFloatingComponents(cell_info, y, x + 1, id);
    }
}

}

BoardInfoDetailed BoardManager::ComputeBoardInfoDetailed(const BoardInfoSimple& info) const {
    // -2 stands for undecided
    Grid<std::pair<BoardInfoDetailed::CellKind, int>> cell_info(height_, width_, {BoardInfoDetailed::CellKind::kEmpty, -2});
    std::vector<std::vector<std::pair<int, int>>> blocks;

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (cell_info.at(y, x).second != -2) continue;
            if (cell(y, x) == Cell::kEmpty) {
                cell_info.at(y, x) = std::make_pair(BoardInfoDetailed::CellKind::kEmpty, -1);
            }
        }
    }

    for (int i = 0; i < info.blocks.num_groups(); ++i) {
        bool has_arrow = false;
        for (auto [y, x] : info.blocks.group(i)) {
            if (problem_.GetArrowId(y, x) >= 0) {
                assert(!has_arrow);
                has_arrow = true;
            }
        }
        if (has_arrow) {
            std::vector<std::pair<int, int>> group;
            for (auto [y, x] : info.blocks.group(i)) {
                cell_info.at(y, x) = std::make_pair(BoardInfoDetailed::CellKind::kBlock, (int)blocks.size());
                group.push_back({y, x});
            }
            blocks.push_back(std::move(group));
        }
    }

    std::vector<std::vector<std::pair<int, int>>> block_neighbors(blocks.size());

    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (cell(y, x) != Cell::kUndecided) continue;

            int neighbor_block_id = -1;
            for (int i = 0; i < 4; ++i) {
                int y2 = y + kFourNeighborY[i];
                int x2 = x + kFourNeighborX[i];
                if (!(0 <= y2 && y2 < height_ && 0 <= x2 && x2 < width_)) continue;

                auto d = cell_info.at(y2, x2);
                if (d.first == BoardInfoDetailed::CellKind::kBlock) {
                    if (neighbor_block_id == -1) {
                        neighbor_block_id = d.second;
                    } else if (neighbor_block_id != d.second) {
                        neighbor_block_id = -2;
                    }
                }
            }

            if (neighbor_block_id >= 0) {
                cell_info.at(y, x) = std::make_pair(BoardInfoDetailed::CellKind::kBlockNeighbor, neighbor_block_id);
                block_neighbors[neighbor_block_id].push_back({y, x});
            } else if (neighbor_block_id == -2) {
                cell_info.at(y, x) = std::make_pair(BoardInfoDetailed::CellKind::kEmpty, -1);
            }
        }
    }

    int num_floatings = 0;
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            if (cell_info.at(y, x).second != -2) continue;
            TraverseFloatingComponents(cell_info, y, x, num_floatings++);
        }
    }

    std::vector<std::vector<std::pair<int, int>>> floatings(num_floatings);
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            auto d = cell_info.at(y, x);
            if (d.first == BoardInfoDetailed::CellKind::kFloating) {
                floatings[d.second].push_back({y, x});
            }
        }
    }

    return BoardInfoDetailed {
        std::move(cell_info),
        std::move(blocks),
        std::move(block_neighbors),
        std::move(floatings),
    };
}

void BoardManager::Dump() const {
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            switch (cell(y, x)) {
                case Cell::kUndecided:
                    printf(". ");
                    continue;
                case Cell::kSquare:
                    printf("# ");
                    continue;
                case Cell::kEmpty:
                    printf("x ");
                    continue;
            }
        }
        puts("");
    }
    puts("");
}

}
