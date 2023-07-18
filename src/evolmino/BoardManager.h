#pragma once

#include <vector>

#include "core/Solver.h"

#include "Grid.h"
#include "Group.h"
#include "evolmino/Problem.h"

namespace evolmino {

// Connectivity information of a puzzle board.
//
// A block is a connected component of orthogonally adjacent "square" cells.
// We obtain the set of potential blocks by assuming that all undecided cells are "square" cells.
struct BoardInfoSimple {
    GroupInfo blocks;
    GroupInfo potential_blocks;
};

struct BoardInfoDetailed {
    enum CellKind {
        // A cell which is decided to be `Cell::kEmpty`. Undecided cells adjacent to
        // different blocks are also included.
        kEmpty,

        // A square cell which is orthogonally connected to an arrow cell.
        kBlock,

        // An undecided cell directly adjacent to a `kBlock` cell.
        kBlockNeighbor,

        // Other square / undecided cell.
        kFloating,
    };

    Grid<std::pair<CellKind, int>> cell_info;
    std::vector<std::vector<std::pair<int, int>>> blocks;
    std::vector<std::vector<std::pair<int, int>>> block_neighbors;
    std::vector<std::vector<std::pair<int, int>>> floatings;
};

class BoardManager {
public:
    enum Cell {
        kUndecided,
        kSquare,
        kEmpty,
    };

    BoardManager(const Problem& problem, Glucose::Var origin);

    int height() const { return height_; }
    int width() const { return width_; }

    Cell cell(int y, int x) const;
    Cell cell(const std::pair<int, int>& p) { return cell(p.first, p.second); }
    Glucose::Var CellVar(int y, int x) const;

    const Problem& problem() const { return problem_; }

    void Decide(Glucose::Lit lit);
    void Undo(Glucose::Lit lit);

    std::vector<Glucose::Var> RelatedVariables() const;

    // Computes the most straightforward "reason", in which all the known decisions are related.
    std::vector<Glucose::Lit> ReasonNaive() const;

    static Glucose::Var AllocateVariables(Glucose::Solver& solver, int height, int width);

    // Compute the reason why cells (ya, xa) and (yb, xb) are connected via square cells
    std::vector<Glucose::Lit> ReasonForPath(int ya, int xa, int yb, int xb) const;

    // Compute the reason prohibiting a potential block from expanding
    std::vector<Glucose::Lit> ReasonForPotentialUnitBoundary(const BoardInfoSimple& info, int potential_group_id) const;

    // Compute the reason why a block is of at least the current size
    std::vector<Glucose::Lit> ReasonForBlock(const BoardInfoDetailed& info, int block_id) const;

    // Compute the reason prohibiting a block from expanding beyond adjacent floating regions
    std::vector<Glucose::Lit> ReasonForAdjacentFloatingBoundary(const BoardInfoDetailed& info, int block_id) const;

    BoardInfoSimple ComputeBoardInfoSimple() const;

    // This function must NOT be called if there exists a block containing more than one arrow cells.
    BoardInfoDetailed ComputeBoardInfoDetailed(const BoardInfoSimple& info) const;

    void Dump() const;

private:
    int height_, width_;
    Problem problem_;
    Glucose::Var origin_;
    std::vector<Cell> cells_;
    std::vector<Glucose::Lit> decisions_;
};

}
