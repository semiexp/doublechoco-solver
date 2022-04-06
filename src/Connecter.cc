#include "Connecter.h"

#include "Grid.h"

Connecter::Connecter(const Problem& problem, Glucose::Var origin)
    : problem_(problem), board_(problem.height(), problem.width(), origin) {}

bool Connecter::initialize(Glucose::Solver& solver) {
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
                              const BoardManager& board, int y, int x, int* group_by_color) {
    if (visited.at(y, x)) {
        return true;
    }
    visited.at(y, x) = true;

    int c = problem.color(y, x);
    int id = group_id.at(y, x);
    if (group_by_color[c] == -1) {
        group_by_color[c] = id;
    } else if (group_by_color[c] != id) {
        return false;
    }

    if (y > 0 && board.vertical(y - 1, x) == BoardManager::Border::kConnected) {
        if (!CheckConnectedComponents(visited, group_id, problem, board, y - 1, x, group_by_color)) {
            return false;
        }
    }
    if (y < visited.height() - 1 && board.vertical(y, x) == BoardManager::Border::kConnected) {
        if (!CheckConnectedComponents(visited, group_id, problem, board, y + 1, x, group_by_color)) {
            return false;
        }
    }
    if (x > 0 && board.horizontal(y, x - 1) == BoardManager::Border::kConnected) {
        if (!CheckConnectedComponents(visited, group_id, problem, board, y, x - 1, group_by_color)) {
            return false;
        }
    }
    if (x < visited.width() - 1 && board.horizontal(y, x) == BoardManager::Border::kConnected) {
        if (!CheckConnectedComponents(visited, group_id, problem, board, y, x + 1, group_by_color)) {
            return false;
        }
    }
    return true;
}

} // namespace

bool Connecter::propagate(Glucose::Solver& solver, Glucose::Lit p) {
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

    // Contradiction if two cells of the same color are connected but have different color
    Grid<bool> visited(height, width, false);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (visited.at(y, x)) {
                continue;
            }
            int group_by_color[2] = {-1, -1};
            if (!CheckConnectedComponents(visited, group_id, problem_, board_, y, x, group_by_color)) {
                return false;
            }
        }
    }

    return true;
}

void Connecter::calcReason(Glucose::Solver& solver, Glucose::Lit p, Glucose::Lit extra,
                           Glucose::vec<Glucose::Lit>& out_reason) {
    board_.calcReasonSimple(p, extra, out_reason);
}

void Connecter::undo(Glucose::Solver& solver, Glucose::Lit p) { board_.Undo(p); }
