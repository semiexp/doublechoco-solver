#pragma once

#include "core/Solver.h"

template <typename T>
class SimplePropagator : public Glucose::Constraint {
public:
    SimplePropagator() = default;
    virtual ~SimplePropagator() = default;

    bool initialize(Glucose::Solver& solver) override final {
        std::vector<Glucose::Var> related_vars = static_cast<T*>(this)->RelatedVariables();

        for (Glucose::Var var : related_vars) {
            solver.addWatch(Glucose::mkLit(var, false), this);
            solver.addWatch(Glucose::mkLit(var, this), this);
        }

        for (Glucose::Var var : related_vars) {
            Glucose::lbool val = solver.value(var);
            if (val == l_True) {
                if (!propagate(solver, Glucose::mkLit(var, false))) {
                    return false;
                }
            } else if (val == l_False) {
                if (!propagate(solver, Glucose::mkLit(var, true))) {
                    return false;
                }
            }
        }

        return true;
    }

    bool propagate(Glucose::Solver& solver, Glucose::Lit p) override final {
        solver.registerUndo(var(p), this);
        static_cast<T*>(this)->SimplePropagatorDecide(p);

        if (num_pending_propagation() > 0) {
            reasons_.push_back({});
            return true;
        }

        auto res = static_cast<T*>(this)->DetectInconsistency();
        if (res.has_value()) {
            reasons_.push_back(*res);
            return false;
        } else {
            reasons_.push_back({});
            return true;
        }
    }

    void calcReason(Glucose::Solver& solver, Glucose::Lit p, Glucose::Lit extra, Glucose::vec<Glucose::Lit>& out_reason) override final {
        assert(!reasons_.back().empty());
        for (auto& lit : reasons_.back()) {
            out_reason.push(lit);
        }
        if (extra != Glucose::lit_Undef) {
            out_reason.push(extra);
        }
    }

    void undo(Glucose::Solver& solver, Glucose::Lit p) override final {
        static_cast<T*>(this)->SimplePropagatorUndo(p);
        reasons_.pop_back();
    }

    // Subclasses should implement the following functions:

    // Returns all variables related to this constraint.
    // std::vector<Glucose::Var> RelatedVariables();

    // Receives the fact that the literal `p` is decided. The propagator can reflect this fact to its internal state.
    // void SimplePropagatorDecide(Glucose::Lit p);

    // Receives the fact that the decision of the literal `p` is undone. It is guaranteed that `Decide` and `Undo` are
    // called in the FIFO manner: the argument `p` to `Undo` is always the last literal among `Decide`d and undone ones.
    // void SimplePropagatorUndo(Glucose::Lit p);

    // Determines whether inconsistency can be found under current decisions. If no inconsistency is found, std::nullopt
    // is returned. If an inconsistenty is found, the "reason" is returned. A reason is a collection of literals
    // all of which cannot be true at the same time for this constraint to be satisfied.
    // std::optional<std::vector<Glucose::Lit>> DetectInconsistency();

private:
    std::vector<std::vector<Glucose::Lit>> reasons_;

};
