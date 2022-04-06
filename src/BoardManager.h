#pragma once

#include "core/Solver.h"

#include <vector>

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

    BoardManager(int height, int width, Glucose::Var origin);

    Border horizontal(int y, int x) const;
    Border vertical(int y, int x) const;
    Glucose::Var HorizontalVar(int y, int x) const;
    Glucose::Var VerticalVar(int y, int x) const;

    void Decide(Glucose::Lit lit);
    void Undo(Glucose::Lit lit);

    std::vector<Glucose::Var> RelatedVariables() const;

    // Computes the most straightforward "reason", in which all the known decisions are related.
    void calcReasonSimple(Glucose::Lit p, Glucose::Lit extra, Glucose::vec<Glucose::Lit>& out_reason);

    static Glucose::Var AllocateVariables(Glucose::Solver& solver, int height, int width);

    void Dump() const;

private:
    int height_, width_;
    Glucose::Var origin_;
    std::vector<Border> horizontal_, vertical_;
    std::vector<Glucose::Lit> decisions_;
};
