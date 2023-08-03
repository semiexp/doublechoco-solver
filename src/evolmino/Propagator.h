#pragma once

#include <optional>

#include "core/Constraint.h"
#include "core/Solver.h"

#include "evolmino/BoardManager.h"
#include "evolmino/Problem.h"
#include "SimplePropagator.h"

namespace evolmino {

class Propagator : public SimplePropagator<Propagator> {
public:
    Propagator(const Problem& problem, Glucose::Var origin);
    virtual ~Propagator() = default;

    std::vector<Glucose::Var> RelatedVariables();
    void SimplePropagatorDecide(Glucose::Lit p);
    void SimplePropagatorUndo(Glucose::Lit p);
    std::optional<std::vector<Glucose::Lit>> DetectInconsistency();

private:
    Problem problem_;
    BoardManager board_;
    std::vector<std::vector<Glucose::Lit>> reasons_;
};

}
