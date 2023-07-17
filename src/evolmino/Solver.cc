#include "evolmino/Solver.h"

#include <map>

#include "core/Solver.h"

#include "evolmino/BoardManager.h"
#include "evolmino/Propagator.h"

namespace evolmino {

namespace {

void AddConstraints(const Problem& problem, Glucose::Solver& solver, Glucose::Var origin) {
    int height = problem.height();
    int width = problem.width();

    solver.addConstraint(std::make_unique<Propagator>(problem, origin));

    // initially placed black cells / squares
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            switch (problem.cell(y, x)) {
                case Problem::Cell::kBlack:
                    solver.addClause(Glucose::mkLit(origin + (y * width + x), true));
                    break;
                case Problem::Cell::kSquare:
                    solver.addClause(Glucose::mkLit(origin + (y * width + x), false));
                    break;
                case Problem::Cell::kEmpty:
                    break;
            }
        }
    }

    // both of adjacent cells in an arrow cannot be squares
    for (int i = 0; i < problem.NumArrows(); ++i) {
        auto& arrow = problem.GetArrow(i);

        for (int j = 1; j < arrow.size(); ++j) {
            int idx_a = arrow[j - 1].first * width + arrow[j - 1].second;
            int idx_b = arrow[j].first * width + arrow[j].second;

            solver.addClause(Glucose::mkLit(origin + idx_a, true), Glucose::mkLit(origin + idx_b, true));
        }
    }

    // at least 2 squares on each arrow
    for (int i = 0; i < problem.NumArrows(); ++i) {
        auto& arrow = problem.GetArrow(i);

        for (int j = 0; j < arrow.size(); ++j) {
            Glucose::vec<Glucose::Lit> clause;
            for (int k = 0; k < arrow.size(); ++k) {
                if (j != k) {
                    int idx = arrow[k].first * width + arrow[k].second;
                    clause.push(Glucose::mkLit(origin + idx, false));
                }
            }
            solver.addClause(clause);
        }
    }
}

}

std::optional<EvolminoAnswer> FindAnswer(const Problem& problem) {
    Glucose::Solver solver;
    Glucose::Var origin = BoardManager::AllocateVariables(solver, problem.height(), problem.width());

    AddConstraints(problem, solver, origin);

    if (!solver.solve())
        return std::nullopt;

    int height = problem.height();
    int width = problem.width();
    EvolminoAnswer ret(height, width, EvolminoAnswerCell::kUndecided);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (solver.modelValue(Glucose::mkLit(origin + y * width + x)) == l_True) {
                ret.at(y, x) = EvolminoAnswerCell::kSquare;
            } else {
                ret.at(y, x) = EvolminoAnswerCell::kEmpty;
            }
        }
    }

    return ret;
}

std::optional<EvolminoAnswer> Solve(const Problem& problem) {
    Glucose::Solver solver;
    Glucose::Var origin = BoardManager::AllocateVariables(solver, problem.height(), problem.width());

    AddConstraints(problem, solver, origin);

    if (!solver.solve())
        return std::nullopt;

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

    int height = problem.height();
    int width = problem.width();
    EvolminoAnswer ret(height, width, EvolminoAnswerCell::kUndecided);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            switch (board.cell(y, x)) {
                case BoardManager::Cell::kUndecided:
                    ret.at(y, x) = EvolminoAnswerCell::kUndecided;
                    break;
                case BoardManager::Cell::kEmpty:
                    ret.at(y, x) = EvolminoAnswerCell::kEmpty;
                    break;
                case BoardManager::Cell::kSquare:
                    ret.at(y, x) = EvolminoAnswerCell::kSquare;
                    break;
            }
        }
    }

    return ret;
}

}
