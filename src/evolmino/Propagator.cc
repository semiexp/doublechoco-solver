#include "evolmino/Propagator.h"

namespace evolmino {

Propagator::Propagator(const Problem& problem, Glucose::Var origin) : problem_(problem), board_(problem, origin) {}

bool Propagator::initialize(Glucose::Solver& solver) {
    std::vector<Glucose::Var> related_vars = board_.RelatedVariables();

    for (Glucose::Var var : related_vars) {
        solver.addWatch(Glucose::mkLit(var, false), this);
        solver.addWatch(Glucose::mkLit(var, this), this);
    }

    for (Glucose::Var var : related_vars) {
        Glucose::lbool val = solver.value(var);
        if (val == l_True) {
            if (!propagate(solver, Glucose::mkLit(var, false))) {
                return false;
            }
        } else if (val == l_False) {
            if (!propagate(solver, Glucose::mkLit(var, true))) {
                return false;
            }
        }
    }

    return true;
}

bool Propagator::propagate(Glucose::Solver& solver, Glucose::Lit p) {
    solver.registerUndo(var(p), this);
    board_.Decide(p);

    if (num_pending_propagation() > 0) {
        reasons_.push_back({});
        return true;
    }

    auto res = DetectInconsistency();
    if (res.has_value()) {
        reasons_.push_back(*res);
        return false;
    } else {
        reasons_.push_back({});
        return true;
    }
}

void Propagator::calcReason(Glucose::Solver& solver, Glucose::Lit p, Glucose::Lit extra,
                            Glucose::vec<Glucose::Lit>& out_reason) {
    assert(!reasons_.back().empty());
    for (auto& lit : reasons_.back()) {
        out_reason.push(lit);
    }
    if (extra != Glucose::lit_Undef) {
        out_reason.push(extra);
    }
}

void Propagator::undo(Glucose::Solver& solver, Glucose::Lit p) {
    board_.Undo(p);
    reasons_.pop_back();
}

namespace {

const int kFourNeighborY[] = {-1, 0, 1, 0};
const int kFourNeighborX[] = {0, -1, 0, 1};

}

std::optional<std::vector<Glucose::Lit>> Propagator::DetectInconsistency() {
    BoardInfoSimple board_info_simple = board_.ComputeBoardInfoSimple();

    // Each block is reachable to an arrow cell
    for (int i = 0; i < board_info_simple.potential_blocks.num_groups(); ++i) {
        std::pair<int, int> square_cell{-1, -1};
        bool has_arrow = false;
        for (auto [y, x] : board_info_simple.potential_blocks.group(i)) {
            if (problem_.GetArrowId(y, x) >= 0) {
                has_arrow = true;
            }
            if (board_.cell(y, x) == BoardManager::Cell::kSquare) {
                square_cell = std::make_pair(y, x);
            }
        }
        if (square_cell.first != -1 && !has_arrow) {
            std::vector<Glucose::Lit> ret = board_.ReasonForPotentialUnitBoundary(board_info_simple, i);
            ret.push_back(Glucose::mkLit(board_.CellVar(square_cell.first, square_cell.second)));
            return ret;
        }
    }

    // Each block does not contain more than one arrow cell
    for (int i = 0; i < board_info_simple.blocks.num_groups(); ++i) {
        std::pair<int, int> arrow_cell{-1, -1};
        for (auto [y, x] : board_info_simple.blocks.group(i)) {
            if (problem_.GetArrowId(y, x) >= 0) {
                if (arrow_cell.first == -1) {
                    arrow_cell = std::make_pair(y, x);
                } else {
                    return board_.ReasonForPath(y, x, arrow_cell.first, arrow_cell.second);
                }
            }
        }
    }

    // Each arrow contains at least 2 blocks
    // (This constraint is represented as SAT clauses)

    BoardInfoDetailed board_info_detail = board_.ComputeBoardInfoDetailed(board_info_simple);

    // If two adjacent blocks X, Y appears in this order along an arrow, Y must be an "extension" of X, that is, 
    // Y can be obtained by adding at least 1 square cells to X (without flipping and rotation).
    // Note that we can add arbitrarily as many square cells.
    for (int i = 0; i < problem_.NumArrows(); ++i) {
        auto& arrow = problem_.GetArrow(i);
        int last_block_id = -1;
        for (int j = 0; j < arrow.size(); ++j) {
            if (board_.cell(arrow[j]) == BoardManager::Cell::kSquare) {
                assert(board_info_detail.cell_info.at(arrow[j]).first == BoardInfoDetailed::CellKind::kBlock);
                int block_id = board_info_detail.cell_info.at(arrow[j]).second;

                std::vector<bool> allowed_floatings(board_info_detail.floatings.size(), false);
                for (auto [y, x] : board_info_detail.block_neighbors[block_id]) {
                    for (int d = 0; d < 4; ++d) {
                        int y2 = y + kFourNeighborY[d];
                        int x2 = x + kFourNeighborX[d];
                        if (!(0 <= y2 && y2 < board_.height() && 0 <= x2 && x2 < board_.width())) continue;
                        auto f = board_info_detail.cell_info.at(y2, x2);
                        if (f.first == BoardInfoDetailed::CellKind::kFloating) {
                            allowed_floatings[f.second] = true;
                        }
                    }
                }

                if (last_block_id != -1) {
                    bool isok = false;
                    auto& last_block = board_info_detail.blocks[last_block_id];
                    assert(!last_block.empty());

                    // TODO: number of candidates can be reduced more
                    for (int y = 0; y < board_.height(); ++y) {
                        for (int x = 0; x < board_.width(); ++x) {
                            // check if `last_block` can be placed so that `last_block[0]` is put on (y, x)
                            bool flg = true;
                            int dy = y - last_block[0].first;
                            int dx = x - last_block[0].second;

                            for (int v = 0; v < last_block.size(); ++v) {
                                int y2 = last_block[v].first + dy;
                                int x2 = last_block[v].second + dx;

                                if (!(0 <= y2 && y2 < board_.height() && 0 <= x2 && x2 < board_.width())) {
                                    flg = false;
                                    break;
                                }

                                auto& d = board_info_detail.cell_info.at(y2, x2);
                                if (!((d.first == BoardInfoDetailed::CellKind::kFloating && allowed_floatings[d.second]) || (d.first != BoardInfoDetailed::CellKind::kFloating && d.second == block_id))) {
                                    flg = false;
                                    break;
                                }
                            }

                            if (flg) {
                                isok = true;
                                break;
                            }
                        }
                        if (isok) break;
                    }

                    if (!isok) {
                        std::vector<Glucose::Lit> ret = board_.ReasonForBlock(board_info_detail, last_block_id);
                        ret.push_back(Glucose::mkLit(board_.CellVar(arrow[j].first, arrow[j].second)));
                        for (auto lit : board_.ReasonForAdjacentFloatingBoundary(board_info_detail, block_id)) {
                            ret.push_back(lit);
                        }
                        return ret;
                    }
                }

                last_block_id = block_id;
            }
        }
    }

    std::vector<int> potential_block_size(board_info_detail.blocks.size(), -1);
    for (int i = 0; i < board_info_detail.blocks.size(); ++i) {
        std::vector<int> neighbor_floatings;
        for (auto [y, x] : board_info_detail.block_neighbors[i]) {
            for (int d = 0; d < 4; ++d) {
                int y2 = y + kFourNeighborY[d];
                int x2 = x + kFourNeighborX[d];
                if (!(0 <= y2 && y2 < board_.height() && 0 <= x2 && x2 < board_.width())) {
                    continue;
                }
                auto f = board_info_detail.cell_info.at(y2, x2);
                if (f.first == BoardInfoDetailed::CellKind::kFloating) {
                    neighbor_floatings.push_back(f.second);
                }
            }
        }
        std::sort(neighbor_floatings.begin(), neighbor_floatings.end());
        neighbor_floatings.erase(std::unique(neighbor_floatings.begin(), neighbor_floatings.end()), neighbor_floatings.end());

        int ub = board_info_detail.blocks[i].size() + board_info_detail.block_neighbors[i].size();
        for (int f : neighbor_floatings) {
            ub += board_info_detail.floatings[f].size();
        }
        potential_block_size[i] = ub;
    }

    for (int i = 0; i < problem_.NumArrows(); ++i) {
        auto& arrow = problem_.GetArrow(i);
        int last_block_idx = -1;  // index in `arrow`

        for (int j = 0; j < arrow.size(); ++j) {
            if (board_.cell(arrow[j]) != BoardManager::Cell::kSquare) {
                continue;
            }
            assert(board_info_detail.cell_info.at(arrow[j]).first == BoardInfoDetailed::CellKind::kBlock);

            if (last_block_idx != -1) {
                int last_block_id = board_info_detail.cell_info.at(arrow[last_block_idx]).second;
                int cur_block_id = board_info_detail.cell_info.at(arrow[j]).second;
                assert(last_block_id != cur_block_id);

                int gap_ub = 1;
                for (int k = last_block_idx + 2; k < j - 1; ++k) {
                    auto c = board_.cell(arrow[k]);
                    assert(c != BoardManager::Cell::kSquare);

                    if (c == BoardManager::Cell::kUndecided) {
                        gap_ub += 1;
                        k += 1;
                    }
                }

                int last_lb = board_info_detail.blocks[last_block_id].size();
                int last_ub = potential_block_size[last_block_id];
                int cur_lb = board_info_detail.blocks[cur_block_id].size();
                int cur_ub = potential_block_size[cur_block_id];

                if (cur_ub < last_lb + 1) {
                    // the last block is too large and the current block cannot be large enough
                    std::vector<Glucose::Lit> ret = board_.ReasonForBlock(board_info_detail, last_block_id);
                    ret.push_back(Glucose::mkLit(board_.CellVar(arrow[j].first, arrow[j].second)));
                    for (auto lit : board_.ReasonForAdjacentFloatingBoundary(board_info_detail, cur_block_id)) {
                        ret.push_back(lit);
                    }
                    return ret;
                }
                if (last_ub + gap_ub < cur_lb) {
                    // the current block is too large and the last block cannot be large enough
                    std::vector<Glucose::Lit> ret = board_.ReasonForBlock(board_info_detail, cur_block_id);
                    ret.push_back(Glucose::mkLit(board_.CellVar(arrow[last_block_idx].first, arrow[last_block_idx].second)));
                    for (auto lit : board_.ReasonForAdjacentFloatingBoundary(board_info_detail, last_block_id)) {
                        ret.push_back(lit);
                    }
                    for (int k = last_block_idx + 2; k < j - 1; ++k) {
                        if (board_.cell(arrow[k]) == BoardManager::Cell::kEmpty) {
                            ret.push_back(Glucose::mkLit(board_.CellVar(arrow[k].first, arrow[k].second), true));
                        }
                    }
                    return ret;
                }
            }

            last_block_idx = j;
        }
    }

    // No inconsistency
    return std::nullopt;
}

}
