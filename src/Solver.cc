#include "Solver.h"

#include <memory>

#include "core/Solver.h"

#include "BoardManager.h"
#include "Connecter.h"
#include "Propagator.h"
#include "ShapeFinder.h"
#include "SizeChecker.h"

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

} // namespace

std::optional<DoublechocoAnswer> FindAnswer(const Problem& problem) {
    Glucose::Solver solver;

    int height = problem.height();
    int width = problem.width();

    Glucose::Var origin = BoardManager::AllocateVariables(solver, problem.height(), problem.width());
    /*
    solver.addConstraint(std::make_unique<ShapeFinder>(problem, origin));
    solver.addConstraint(std::make_unique<Connecter>(problem, origin));
    solver.addConstraint(std::make_unique<SizeChecker>(problem, origin));
    */
    solver.addConstraint(std::make_unique<Propagator>(problem, origin));

    // Rough check to forbid unnecessary borders
    // TODO: add a compehensive check
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

    if (!solver.solve())
        return std::nullopt;

    DoublechocoAnswer ret;
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

    /*
    Glucose::vec<Glucose::Lit> clause;
    for (Glucose::Var v : board.RelatedVariables()) {
        clause.push(Glucose::mkLit(v, solver.modelValue(v) == l_True));
    }
    solver.addClause(clause);
    printf("has another answer: %d\n", solver.solve());
    */

    return ret;
}
