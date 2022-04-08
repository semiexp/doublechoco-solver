#include "SizeChecker.h"

#include "Grid.h"

SizeChecker::SizeChecker(const Problem& problem, Glucose::Var origin) : problem_(problem), board_(problem, origin) {}

bool SizeChecker::initialize(Glucose::Solver& solver) {
    std::vector<Glucose::Var> related_vars = board_.RelatedVariables();

    for (Glucose::Var var : related_vars) {
        solver.addWatch(Glucose::mkLit(var, false), this);
        solver.addWatch(Glucose::mkLit(var, true), this);
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

namespace {

void SetGroupId(Grid<int>& group_id, const Problem& problem, const BoardManager& board, int y, int x, int color,
                int id) {
    if (problem.color(y, x) != color) {
        return;
    }
    if (group_id.at(y, x) != -1) {
        return;
    }
    group_id.at(y, x) = id;

    if (y > 0 && board.vertical(y - 1, x) != BoardManager::Border::kWall) {
        SetGroupId(group_id, problem, board, y - 1, x, color, id);
    }
    if (y < group_id.height() - 1 && board.vertical(y, x) != BoardManager::BoardManager::kWall) {
        SetGroupId(group_id, problem, board, y + 1, x, color, id);
    }
    if (x > 0 && board.horizontal(y, x - 1) != BoardManager::Border::kWall) {
        SetGroupId(group_id, problem, board, y, x - 1, color, id);
    }
    if (x < group_id.width() - 1 && board.horizontal(y, x) != BoardManager::Border::kWall) {
        SetGroupId(group_id, problem, board, y, x + 1, color, id);
    }
}

bool CheckConnectedComponents(Grid<bool>& visited, const Grid<int>& group_id, const Problem& problem,
                              const BoardManager& board, int y, int x, int* group_by_color, int* size_by_color,
                              int& clue_num) {
    if (visited.at(y, x)) {
        return true;
    }
    visited.at(y, x) = true;

    int c = problem.color(y, x);
    ++size_by_color[c];
    int n = problem.num(y, x);
    if (n != -1) {
        if (clue_num == -1) {
            clue_num = n;
        } else if (clue_num != n) {
            return false;
        }
    }
    int id = group_id.at(y, x);
    if (group_by_color[c] == -1) {
        group_by_color[c] = id;
    } else if (group_by_color[c] != id) {
        return false;
    }

    if (y > 0 && board.vertical(y - 1, x) == BoardManager::Border::kConnected) {
        if (!CheckConnectedComponents(visited, group_id, problem, board, y - 1, x, group_by_color, size_by_color,
                                      clue_num)) {
            return false;
        }
    }
    if (y < visited.height() - 1 && board.vertical(y, x) == BoardManager::Border::kConnected) {
        if (!CheckConnectedComponents(visited, group_id, problem, board, y + 1, x, group_by_color, size_by_color,
                                      clue_num)) {
            return false;
        }
    }
    if (x > 0 && board.horizontal(y, x - 1) == BoardManager::Border::kConnected) {
        if (!CheckConnectedComponents(visited, group_id, problem, board, y, x - 1, group_by_color, size_by_color,
                                      clue_num)) {
            return false;
        }
    }
    if (x < visited.width() - 1 && board.horizontal(y, x) == BoardManager::Border::kConnected) {
        if (!CheckConnectedComponents(visited, group_id, problem, board, y, x + 1, group_by_color, size_by_color,
                                      clue_num)) {
            return false;
        }
    }
    return true;
}

} // namespace

bool SizeChecker::propagate(Glucose::Solver& solver, Glucose::Lit p) {
    solver.registerUndo(var(p), this);
    board_.Decide(p);

    int height = problem_.height();
    int width = problem_.width();

    // Cells which can be potentially connected by cells of the same color
    Grid<int> group_id(height, width, -1);
    int id_last = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (group_id.at(y, x) != -1) {
                continue;
            }
            SetGroupId(group_id, problem_, board_, y, x, problem_.color(y, x), id_last++);
        }
    }

    std::vector<int> group_size(id_last, 0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            ++group_size[group_id.at(y, x)];
        }
    }

    Grid<bool> visited(height, width, false);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (visited.at(y, x)) {
                continue;
            }
            int size_by_color[2] = {0, 0};
            int group_by_color[2] = {-1, -1};
            int clue_num = -1;
            if (!CheckConnectedComponents(visited, group_id, problem_, board_, y, x, group_by_color, size_by_color,
                                          clue_num)) {
                // Different clue in a connected component
                return false;
            }
            // Connected component of a color is unconditionally larger than that of the another color
            if (group_by_color[0] != -1 && group_size[group_by_color[0]] < size_by_color[1]) {
                return false;
            }
            if (group_by_color[1] != -1 && group_size[group_by_color[1]] < size_by_color[0]) {
                return false;
            }
            if (clue_num != -1) {
                if (clue_num < size_by_color[0] || clue_num < size_by_color[1]) {
                    // Connected component larger than the clue number
                    return false;
                }

                // Possible connected component size smaller than the clue number
                if (group_by_color[0] != -1 && clue_num > group_size[group_by_color[0]]) {
                    return false;
                }
                if (group_by_color[1] != -1 && clue_num > group_size[group_by_color[1]]) {
                    return false;
                }
            }
        }
    }

    return true;
}

void SizeChecker::calcReason(Glucose::Solver& solver, Glucose::Lit p, Glucose::Lit extra,
                             Glucose::vec<Glucose::Lit>& out_reason) {
    board_.calcReasonSimple(p, extra, out_reason);
}

void SizeChecker::undo(Glucose::Solver& solver, Glucose::Lit p) { board_.Undo(p); }
