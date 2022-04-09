#include "Balancer.h"

#include <algorithm>
#include <set>

#include "Grid.h"

namespace {

class WeightedUnionFind {
public:
    WeightedUnionFind(const std::vector<int>& weight) : parent_(weight.size(), -1), weight_(weight) {}

    int Root(int p) {
        if (parent_[p] < 0) {
            return p;
        } else {
            return parent_[p] = Root(parent_[p]);
        }
    }

    bool Union(int p, int q) {
        p = Root(p);
        q = Root(q);

        if (p == q) {
            return false;
        }

        if (parent_[p] > parent_[q])
            std::swap(p, q);
        parent_[p] += parent_[q];
        parent_[q] = p;
        weight_[p] += weight_[q];
        return true;
    }

    int Weight(int p) { return weight_[Root(p)]; }

private:
    std::vector<int> parent_, weight_;
};

} // namespace

Balancer::Balancer(const Problem& problem, Glucose::Var origin)
    : problem_(problem), origin_(origin), adj_edges_(problem.height() * problem.width()),
      edge_deactivated_(problem.height() * (problem.width() - 1) + (problem.height() - 1) * problem.width(), 0),
      color_(problem.height() * problem.width()), rank_(problem.height() * problem.width()),
      lowlink_(problem.height() * problem.width()), subtree_sum_(problem.height() * problem.width()) {
    int height = problem.height();
    int width = problem.width();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width - 1; ++x) {
            edges_.push_back({y * width + x, y * width + (x + 1)});
        }
    }
    for (int y = 0; y < height - 1; ++y) {
        for (int x = 0; x < width; ++x) {
            edges_.push_back({y * width + x, (y + 1) * width + x});
        }
    }
    for (int i = 0; i < edges_.size(); ++i) {
        auto [u, v] = edges_[i];
        adj_edges_[u].push_back({i, v});
        adj_edges_[v].push_back({i, u});
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            color_[y * width + x] = problem_.color(y, x) * 2 - 1;
        }
    }
}

bool Balancer::initialize(Glucose::Solver& solver) {
    std::vector<Glucose::Var> related_vars;
    for (int i = 0; i < adj_edges_.size(); ++i) {
        Glucose::Var var = origin_ + i;
        solver.addWatch(Glucose::mkLit(var, false), this);
    }

    for (int i = 0; i < adj_edges_.size(); ++i) {
        Glucose::Var var = origin_ + i;
        Glucose::lbool val = solver.value(var);
        if (val == l_True) {
            if (!propagate(solver, Glucose::mkLit(var, false))) {
                return false;
            }
        }
    }

    return true;
}

bool Balancer::propagate(Glucose::Solver& solver, Glucose::Lit p) {
    solver.registerUndo(var(p), this);
    assert(!Glucose::sign(p));
    int edge_id = Glucose::var(p) - origin_;
    decision_order_.push_back(edge_id);
    assert(edge_deactivated_[edge_id] == 0);
    edge_deactivated_[edge_id] = 1;

    if (num_pending_propagation() > 0) {
        // lazy propagation
        return true;
    }

    std::fill(rank_.begin(), rank_.end(), -1);
    next_rank_ = 0;
    for (int v = 0; v < rank_.size(); ++v) {
        if (rank_[v] != -1) {
            continue;
        }
        traverse(v, -1);
        if (subtree_sum_[v] != 0) {
            // the connected component containing `i` is not balanced
            return false;
        }
    }

    for (int i = 0; i < edges_.size(); ++i) {
        if (edge_deactivated_[i]) {
            continue;
        }
        auto [u, v] = edges_[i];
        if (rank_[u] > rank_[v]) {
            std::swap(u, v);
        }
        if (lowlink_[v] > rank_[u]) {
            // v is separated from u after removal of edge (u, v)
            if (subtree_sum_[v] != 0) {
                // removing (u, v) makes the graph imbalance
                if (!solver.enqueue(Glucose::mkLit(origin_ + i, true), this)) {
                    return false;
                }
            }
        }
    }

    return true;
}

void Balancer::traverse(int u, int parent) {
    rank_[u] = next_rank_++;
    lowlink_[u] = rank_[u];
    subtree_sum_[u] = color_[u];
    for (auto [edge_id, v] : adj_edges_[u]) {
        if (edge_deactivated_[edge_id]) {
            continue;
        }
        if (v == parent) {
            continue;
        }
        if (rank_[v] == -1) {
            traverse(v, u);
            subtree_sum_[u] += subtree_sum_[v];
            lowlink_[u] = std::min(lowlink_[u], lowlink_[v]);
        } else {
            lowlink_[u] = std::min(lowlink_[u], rank_[v]);
        }
    }
}

void Balancer::calcReason(Glucose::Solver& solver, Glucose::Lit p, Glucose::Lit extra,
                          Glucose::vec<Glucose::Lit>& out_reason) {
    if (p != Glucose::lit_Undef) {
        assert(Glucose::sign(p));
        int edge_id = Glucose::var(p) - origin_;
        assert(!edge_deactivated_[edge_id]);
        edge_deactivated_[edge_id] = 1;
    }
    if (extra != Glucose::lit_Undef) {
        assert(!Glucose::sign(extra));
        int edge_id = Glucose::var(extra) - origin_;
        assert(!edge_deactivated_[edge_id]);
        edge_deactivated_[edge_id] = 1;
        decision_order_.push_back(edge_id);
    }

    std::vector<Glucose::Lit> reason = CalcReasonImpl();
    for (auto l : reason) {
        out_reason.push(l);
    }

    if (p != Glucose::lit_Undef) {
        int edge_id = Glucose::var(p) - origin_;
        edge_deactivated_[edge_id] = 0;
    }
    if (extra != Glucose::lit_Undef) {
        int edge_id = Glucose::var(extra) - origin_;
        edge_deactivated_[edge_id] = 0;
        decision_order_.pop_back();
    }
}

namespace {

int dfs(const Problem& problem, Grid<bool>& visited, const std::set<int>& del_edge, int y, int x) {
    if (visited.at(y, x))
        return 0;
    visited.at(y, x) = true;
    int ret = problem.color(y, x) * 2 - 1;
    if (y > 0 && del_edge.count(problem.height() * (problem.width() - 1) + (y - 1) * problem.width() + x) == 0) {
        ret += dfs(problem, visited, del_edge, y - 1, x);
    }
    if (y < problem.height() - 1 &&
        del_edge.count(problem.height() * (problem.width() - 1) + y * problem.width() + x) == 0) {
        ret += dfs(problem, visited, del_edge, y + 1, x);
    }
    if (x > 0 && del_edge.count(y * (problem.width() - 1) + x - 1) == 0) {
        ret += dfs(problem, visited, del_edge, y, x - 1);
    }
    if (x < problem.width() - 1 && del_edge.count(y * (problem.width() - 1) + x) == 0) {
        ret += dfs(problem, visited, del_edge, y, x + 1);
    }
    return ret;
}

void is_disconnected(const Problem& problem, std::vector<Glucose::Lit> reason, Glucose::Var origin) {
    std::set<int> del_edge;
    for (auto& l : reason) {
        del_edge.insert(Glucose::var(l) - origin);
    }
    Grid<bool> visited(problem.height(), problem.width(), false);
    for (int y = 0; y < problem.height(); ++y) {
        for (int x = 0; x < problem.width(); ++x) {
            if (visited.at(y, x))
                continue;
            if (dfs(problem, visited, del_edge, y, x) != 0)
                return;
        }
    }
    for (int y = 0; y < problem.height(); ++y) {
        for (int x = 0; x < problem.width(); ++x) {
            printf("%d ", problem.color(y, x));
        }
        puts("");
        puts("");
    }
    for (auto e : del_edge)
        printf("%d ", e);
    puts("");
    assert(false);
}

} // namespace

std::vector<Glucose::Lit> Balancer::CalcReasonImpl() {
    WeightedUnionFind uf(color_);

    for (int i = 0; i < edges_.size(); ++i) {
        if (!edge_deactivated_[i]) {
            uf.Union(edges_[i].first, edges_[i].second);
        }
    }

    int n_imbalance = 0;
    for (int i = 0; i < color_.size(); ++i) {
        if (uf.Root(i) == i && uf.Weight(i) != 0) {
            ++n_imbalance;
        }
    }

    assert(n_imbalance > 0);

    std::vector<Glucose::Lit> ret;
    for (int i = decision_order_.size() - 1; i >= 0; --i) {
        auto [u, v] = edges_[decision_order_[i]];

        u = uf.Root(u);
        v = uf.Root(v);
        if (u == v) {
            continue;
        }

        int weight_u = uf.Weight(u), weight_v = uf.Weight(v);
        if (weight_u != 0 && weight_u + weight_v == 0 && n_imbalance == 2) {
            // keep this edge
            ret.push_back(Glucose::mkLit(origin_ + decision_order_[i], false));
        } else {
            uf.Union(u, v);
            n_imbalance += (weight_u == 0 ? 0 : -1) + (weight_v == 0 ? 0 : -1) + ((weight_u + weight_v) == 0 ? 0 : 1);
        }
    }
    return ret;
}

void Balancer::undo(Glucose::Solver& solver, Glucose::Lit p) {
    assert(!decision_order_.empty());
    assert(decision_order_.back() == Glucose::var(p) - origin_);
    int e = decision_order_.back();
    assert(edge_deactivated_[e] == 1);
    edge_deactivated_[e] = 0;
    decision_order_.pop_back();
}
