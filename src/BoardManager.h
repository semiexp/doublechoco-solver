#pragma once

#include "core/Solver.h"

#include "Grid.h"
#include "Problem.h"

#include <algorithm>
#include <vector>

class GroupInfo {
public:
    GroupInfo(Grid<int>&& group_id);

    int group_id(int y, int x) const { return group_id_.at(y, x); }
    const std::vector<std::vector<std::pair<int, int>>>& groups() const { return groups_; }
    int num_groups() const { return groups_.size(); }
    const std::vector<std::pair<int, int>>& group(int id) const { return groups_[id]; };

private:
    Grid<int> group_id_;
    std::vector<std::vector<std::pair<int, int>>> groups_;
};

// Connectivity information of a puzzle board.
// "unit": connected components consists of cells of one color (connections between differently colored cells are
// ignored). "block": connected components consists of cells of both colors. "potential" means the connected components
// are computed assuming that all undecided borders are connecting adjacent cells.
struct BoardInfo {
    GroupInfo units;
    GroupInfo blocks;
    GroupInfo potential_units;
};

// This solver uses H * (W - 1) + (H - 1) * W variables to represent answers.
// The first H * (W - 1) variables correspond to "horizontal" connections, and the remaining to "vertical".
// For each variable, "true" means that there is a border at the corresponding location, and "false" means that the two
// cells adjacent to the corresponding location are connected.
class BoardManager {
public:
    enum Border {
        kUndecided,
        kWall,
        kConnected,
    };

    BoardManager(const Problem& problem, Glucose::Var origin);

    int height() const { return height_; }
    int width() const { return width_; }

    Border horizontal(int y, int x) const;
    Border vertical(int y, int x) const;
    Glucose::Var HorizontalVar(int y, int x) const;
    Glucose::Var VerticalVar(int y, int x) const;

    const Problem& problem() const { return problem_; }

    void Decide(Glucose::Lit lit);
    void Undo(Glucose::Lit lit);

    std::vector<Glucose::Var> RelatedVariables() const;

    // Compute the reason (a set of literals) building a block
    std::vector<Glucose::Lit> ReasonForBlock(const BoardInfo& info, int block_id) const;

    // Compute the reason prohibiting a potential unit from expanding
    std::vector<Glucose::Lit> ReasonForPotentialUnitBoundary(const BoardInfo& info, int potential_unit_id) const;

    // Computes the most straightforward "reason", in which all the known decisions are related.
    void calcReasonSimple(Glucose::Lit p, Glucose::Lit extra, Glucose::vec<Glucose::Lit>& out_reason);

    static Glucose::Var AllocateVariables(Glucose::Solver& solver, int height, int width);

    BoardInfo ComputeBoardInfo() const;

    void Dump() const;

private:
    int height_, width_;
    Problem problem_;
    Glucose::Var origin_;
    std::vector<Border> horizontal_, vertical_;
    std::vector<Glucose::Lit> decisions_;
};
