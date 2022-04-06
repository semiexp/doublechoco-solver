#include "Solver.h"

#include <memory>

#include "core/Solver.h"

#include "BoardManager.h"
#include "ShapeFinder.h"

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

    Glucose::Var origin = BoardManager::AllocateVariables(solver, problem.height(), problem.width());
    solver.addConstraint(std::make_unique<ShapeFinder>(problem, origin));

    if (!solver.solve())
        return std::nullopt;

    int height = problem.height();
    int width = problem.width();
    DoublechocoAnswer ret;
    ret.horizontal = std::vector<std::vector<DoublechocoAnswer::Border>>(height);
    ret.vertical = std::vector<std::vector<DoublechocoAnswer::Border>>(height - 1);

    BoardManager board(height, width, origin);
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
