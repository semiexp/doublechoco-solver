#pragma once

#include <vector>

#include "core/Constraint.h"
#include "core/Solver.h"

#include "Problem.h"

namespace doublechoco {

// Ensure that all potential block have the equal number of black and white cells
class Balancer : public Glucose::Constraint {
public:
    Balancer(const Problem& problem, Glucose::Var origin);
    virtual ~Balancer() = default;

    bool initialize(Glucose::Solver& solver) override;
    bool propagate(Glucose::Solver& solver, Glucose::Lit p) override;
    void calcReason(Glucose::Solver& solver, Glucose::Lit p, Glucose::Lit extra,
                    Glucose::vec<Glucose::Lit>& out_reason) override;
    void undo(Glucose::Solver& solver, Glucose::Lit p) override;

private:
    void traverse(int u, int parent);

    std::vector<Glucose::Lit> CalcReasonImpl();

    Problem problem_;
    Glucose::Var origin_;
    std::vector<std::vector<std::pair<int, int>>> adj_edges_;
    std::vector<std::pair<int, int>> edges_;
    std::vector<int> edge_deactivated_;
    std::vector<int> color_;
    std::vector<int> rank_, lowlink_, subtree_sum_;
    std::vector<int> decision_order_;
    int next_rank_;
};

}
