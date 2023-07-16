#include "doublechoco/Solver.h"

#include <memory>

#include <map>

#include "core/Solver.h"

#include "doublechoco/Balancer.h"
#include "doublechoco/BoardManager.h"
#include "doublechoco/Propagator.h"

namespace doublechoco {

namespace {

DoublechocoAnswer::Border ConvertBorder(BoardManager::Border b) {
    switch (b) {
    case BoardManager::Border::kUndecided:
        return DoublechocoAnswer::Border::kUndecided;
    case BoardManager::Border::kWall:
        return DoublechocoAnswer::Border::kWall;
    case BoardManager::Border::kConnected:
        return DoublechocoAnswer::Border::kConnected;
    }
    abort();
}

void AddConstraints(const Problem& problem, Glucose::Solver& solver, Glucose::Var origin) {
    int height = problem.height();
    int width = problem.width();

    solver.addConstraint(std::make_unique<Propagator>(problem, origin));
    // TODO: Balancer is unused because it makes the solver slow
    // solver.addConstraint(std::make_unique<Balancer>(problem, origin));

    // Rough check to forbid unnecessary borders
    for (int y = 0; y < height - 1; ++y) {
        for (int x = 0; x < width - 1; ++x) {
            std::vector<Glucose::Var> vars;
            vars.push_back(origin + y * (width - 1) + x);
            vars.push_back(origin + (y + 1) * (width - 1) + x);
            vars.push_back(origin + height * (width - 1) + y * width + x);
            vars.push_back(origin + height * (width - 1) + y * width + (x + 1));
            // !v[i] & !v[j] & !v[k] => !v[l]
            for (int i = 0; i < 4; ++i) {
                Glucose::vec<Glucose::Lit> clause;
                for (int j = 0; j < 4; ++j) {
                    clause.push(Glucose::mkLit(vars[j], i == j));
                }
                solver.addClause(clause);
            }
        }
    }
}

} // namespace

std::optional<DoublechocoAnswer> FindAnswer(const Problem& problem) {
    Glucose::Solver solver;
    Glucose::Var origin = BoardManager::AllocateVariables(solver, problem.height(), problem.width());

    AddConstraints(problem, solver, origin);

    if (!solver.solve())
        return std::nullopt;

    DoublechocoAnswer ret;
    int height = problem.height();
    int width = problem.width();
    ret.horizontal = std::vector<std::vector<DoublechocoAnswer::Border>>(height);
    ret.vertical = std::vector<std::vector<DoublechocoAnswer::Border>>(height - 1);

    BoardManager board(problem, origin);
    for (Glucose::Var v : board.RelatedVariables()) {
        board.Decide(Glucose::mkLit(v, solver.modelValue(v) == l_False));
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width - 1; ++x) {
            ret.horizontal[y].push_back(ConvertBorder(board.horizontal(y, x)));
        }
    }
    for (int y = 0; y < height - 1; ++y) {
        for (int x = 0; x < width; ++x) {
            ret.vertical[y].push_back(ConvertBorder(board.vertical(y, x)));
        }
    }

    return ret;
}

std::optional<DoublechocoAnswer> Solve(const Problem& problem) {
    Glucose::Solver solver;
    Glucose::Var origin = BoardManager::AllocateVariables(solver, problem.height(), problem.width());

    AddConstraints(problem, solver, origin);

    if (!solver.solve()) {
        return std::nullopt;
    }

    BoardManager board(problem, origin);
    std::vector<Glucose::Var> related_vars = board.RelatedVariables();

    std::map<Glucose::Var, bool> assignment;
    for (auto v : related_vars) {
        assignment.emplace(v, solver.modelValue(v) == l_True);
    }

    for (;;) {
        Glucose::vec<Glucose::Lit> refutation;
        for (auto [var, val] : assignment) {
            refutation.push(Glucose::mkLit(var, val));
        }
        solver.addClause(refutation);

        if (!solver.solve()) {
            break;
        }
        for (auto it = assignment.begin(); it != assignment.end();) {
            if ((solver.modelValue(it->first) == l_True) != it->second) {
                it = assignment.erase(it);
            } else {
                ++it;
            }
        }
    }

    for (auto [var, val] : assignment) {
        board.Decide(Glucose::mkLit(var, !val));
    }

    DoublechocoAnswer ret;
    int height = problem.height();
    int width = problem.width();
    ret.horizontal = std::vector<std::vector<DoublechocoAnswer::Border>>(height);
    ret.vertical = std::vector<std::vector<DoublechocoAnswer::Border>>(height - 1);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width - 1; ++x) {
            ret.horizontal[y].push_back(ConvertBorder(board.horizontal(y, x)));
        }
    }
    for (int y = 0; y < height - 1; ++y) {
        for (int x = 0; x < width; ++x) {
            ret.vertical[y].push_back(ConvertBorder(board.vertical(y, x)));
        }
    }
    return ret;
}

}
