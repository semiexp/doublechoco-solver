#pragma once

#include "core/Constraint.h"
#include "core/Solver.h"

#include "BoardManager.h"
#include "Problem.h"

class ShapeFinder : public Glucose::Constraint {
public:
    ShapeFinder(const Problem& problem, Glucose::Var origin);
    virtual ~ShapeFinder() = default;

    bool initialize(Glucose::Solver& solver) override;
    bool propagate(Glucose::Solver& solver, Glucose::Lit p) override;
    void calcReason(Glucose::Solver& solver, Glucose::Lit p, Glucose::Lit extra,
                    Glucose::vec<Glucose::Lit>& out_reason) override;
    void undo(Glucose::Solver& solver, Glucose::Lit p) override;

private:
    Problem problem_;
    BoardManager board_;
    std::vector<std::vector<Glucose::Lit>> reasons_;
};
