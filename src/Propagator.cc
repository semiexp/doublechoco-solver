#include "Propagator.h"

#include <set>

#include "Grid.h"

Propagator::Propagator(const Problem& problem, Glucose::Var origin) : problem_(problem), board_(problem, origin) {}

bool Propagator::initialize(Glucose::Solver& solver) {
    std::vector<Glucose::Var> related_vars = board_.RelatedVariables();

    for (Glucose::Var var : related_vars) {
        solver.addWatch(Glucose::mkLit(var, false), this);
        solver.addWatch(Glucose::mkLit(var, true), this);
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

bool Propagator::propagate(Glucose::Solver& solver, Glucose::Lit p) {
    solver.registerUndo(var(p), this);
    board_.Decide(p);

    if (num_pending_propagation() > 0) {
        reasons_.push_back({});
        return true;
    }

    auto res = DetectInconsistency();
    if (res.has_value()) {
        reasons_.push_back(*res);
        return false;
    } else {
        reasons_.push_back({});
        return true;
    }
}

void Propagator::calcReason(Glucose::Solver& solver, Glucose::Lit p, Glucose::Lit extra,
                            Glucose::vec<Glucose::Lit>& out_reason) {
    if (reasons_.back().empty()) {
        board_.calcReasonSimple(p, extra, out_reason);
    } else {
        for (auto& lit : reasons_.back()) {
            out_reason.push(lit);
        }
        if (extra != Glucose::lit_Undef) {
            out_reason.push(extra);
        }
    }
}

void Propagator::undo(Glucose::Solver& solver, Glucose::Lit p) {
    board_.Undo(p);
    reasons_.pop_back();
}

namespace {

std::pair<std::vector<std::pair<int, int>>, std::vector<std::pair<int, int>>>
Transform(const std::vector<std::pair<int, int>>& group, const std::vector<std::pair<int, int>>& connections, int yk,
          int xk, int flip) {
    auto trans = [&](int y, int x) {
        y *= yk;
        x *= xk;
        if (flip) {
            std::swap(y, x);
        }
        return std::make_pair(y, x);
    };

    std::vector<std::pair<int, int>> group_new, connections_new;
    for (auto& [y, x] : group) {
        group_new.push_back(trans(y, x));
    }
    for (auto& [y, x] : connections) {
        connections_new.push_back(trans(y, x));
    }

    std::pair<int, int> min_pos{std::numeric_limits<int>::max(), std::numeric_limits<int>::max()};
    for (auto& [y, x] : group_new) {
        min_pos = std::min(min_pos, {y, x});
    }
    for (auto& [y, x] : group_new) {
        y -= min_pos.first;
        x -= min_pos.second;
    }
    for (auto& [y, x] : connections_new) {
        y -= min_pos.first * 2;
        x -= min_pos.second * 2;
    }
    std::sort(group_new.begin(), group_new.end());
    return {group_new, connections_new};
}

std::vector<std::pair<std::vector<std::pair<int, int>>, std::vector<std::pair<int, int>>>>
EnumerateTransforms(const std::vector<std::pair<int, int>>& group,
                    const std::vector<std::pair<int, int>>& connections) {
    std::vector<std::pair<std::vector<std::pair<int, int>>, std::vector<std::pair<int, int>>>> ret;
    for (int i = 0; i < 8; ++i) {
        auto transformed = Transform(group, connections, (i >> 2) * 2 - 1, ((i >> 1) & 1) * 2 - 1, i & 1);
        ret.push_back(transformed);
    }
    std::sort(ret.begin(), ret.end());
    ret.erase(std::unique(ret.begin(), ret.end()), ret.end());
    return ret;
}

} // namespace

std::optional<std::vector<Glucose::Lit>> Propagator::DetectInconsistency() {
    int height = problem_.height();
    int width = problem_.width();
    BoardInfo info = board_.ComputeBoardInfo();

    // connecter & size checker
    for (int i = 0; i < info.blocks.num_groups(); ++i) {
        int num = -1;
        bool has_num[2] = {false, false};
        int size_by_color[2] = {0, 0};
        int potential_unit_id[2] = {-1, -1};
        for (auto [y, x] : info.blocks.group(i)) {
            int c = problem_.color(y, x);
            int pb_id = info.potential_units.group_id(y, x);
            if (potential_unit_id[c] == -1) {
                potential_unit_id[c] = pb_id;
            } else if (potential_unit_id[c] != pb_id) {
                // Multiple units of the same color in a block
                // TODO: compute more refined reason
                auto ret = board_.ReasonForBlock(info, i);
                auto app = board_.ReasonForPotentialUnitBoundary(info, pb_id);
                ret.insert(ret.end(), app.begin(), app.end());
                return ret;
            }

            ++size_by_color[c];
            int n = problem_.num(y, x);
            if (n > 0) {
                has_num[c] = true;
                if (num == -1) {
                    num = n;
                } else if (num != n) {
                    // Different clue numbers in a block
                    // TODO: compute more refined reason (path connecting <num> and (y, x))
                    return board_.ReasonForBlock(info, i);
                }
            }
        }

        // Connected component of a color is unconditionally larger than that of the another color
        if (potential_unit_id[0] != -1 && info.potential_units.group(potential_unit_id[0]).size() < size_by_color[1]) {
            auto ret = board_.ReasonForBlock(info, i);
            auto app = board_.ReasonForPotentialUnitBoundary(info, potential_unit_id[0]);
            ret.insert(ret.end(), app.begin(), app.end());
            return ret;
        }
        if (potential_unit_id[1] != -1 && info.potential_units.group(potential_unit_id[1]).size() < size_by_color[0]) {
            auto ret = board_.ReasonForBlock(info, i);
            auto app = board_.ReasonForPotentialUnitBoundary(info, potential_unit_id[1]);
            ret.insert(ret.end(), app.begin(), app.end());
            return ret;
        }

        if (num != -1) {
            // Connected component larger than the clue number
            if (num < size_by_color[0] || num < size_by_color[1]) {
                return board_.ReasonForBlock(info, i);
            }

            // Possible connected component size smaller than the clue number
            // TODO: compute more refined reason
            if (potential_unit_id[0] != -1 && num > info.potential_units.group(potential_unit_id[0]).size()) {
                auto ret = board_.ReasonForPotentialUnitBoundary(info, potential_unit_id[0]);
                if (!has_num[0]) {
                    auto app = board_.ReasonForBlock(info, i);
                    ret.insert(ret.end(), app.begin(), app.end());
                }
                return ret;
            }
            if (potential_unit_id[1] != -1 && num > info.potential_units.group(potential_unit_id[1]).size()) {
                auto ret = board_.ReasonForPotentialUnitBoundary(info, potential_unit_id[1]);
                if (!has_num[1]) {
                    auto app = board_.ReasonForBlock(info, i);
                    ret.insert(ret.end(), app.begin(), app.end());
                }
                return ret;
            }
        }
    }

    // shape finder
    std::set<std::pair<int, int>> adjacent_potential_units_set;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (y < height - 1 && problem_.color(y, x) != problem_.color(y + 1, x) &&
                board_.vertical(y, x) != BoardManager::Border::kWall) {
                int i = info.potential_units.group_id(y, x);
                int j = info.potential_units.group_id(y + 1, x);
                adjacent_potential_units_set.insert({i, j});
                adjacent_potential_units_set.insert({j, i});
            }
            if (x < width - 1 && problem_.color(y, x) != problem_.color(y, x + 1) &&
                board_.horizontal(y, x) != BoardManager::Border::kWall) {
                int i = info.potential_units.group_id(y, x);
                int j = info.potential_units.group_id(y, x + 1);
                adjacent_potential_units_set.insert({i, j});
                adjacent_potential_units_set.insert({j, i});
            }
        }
    }
    std::vector<std::vector<int>> adjacent_potential_units(info.potential_units.num_groups());
    for (auto [i, j] : adjacent_potential_units_set) {
        adjacent_potential_units[i].push_back(j);
    }

    for (int i = 0; i < info.units.num_groups(); ++i) {
        // Find the same shape (of the opposite color) in a neighboring potential unit
        std::vector<std::pair<int, int>> connections;
        for (auto [y, x] : info.units.group(i)) {
            if (y < height - 1 && info.units.group_id(y + 1, x) == i) {
                connections.push_back({y * 2 + 1, x * 2});
            }
            if (x < width - 1 && info.units.group_id(y, x + 1) == i) {
                connections.push_back({y * 2, x * 2 + 1});
            }
        }

        std::pair<int, int> one_cell = info.units.group(i)[0];
        std::vector<std::pair<int, int>> origins;
        int potential_unit_id = info.potential_units.group_id(one_cell.first, one_cell.second);
        for (int g : adjacent_potential_units[potential_unit_id]) {
            for (auto p : info.potential_units.group(g)) {
                origins.push_back(p);
            }
        }

        auto transforms = EnumerateTransforms(info.units.group(i), connections);
        bool found = false;

        std::set<Glucose::Lit> blockers;
        for (auto& [t_group, t_connections] : transforms) {
            for (auto [origin_y, origin_x] : origins) {
                bool invalid = false;
                std::optional<Glucose::Lit> blocker_cand;

                for (auto [dy, dx] : t_connections) {
                    int py = origin_y * 2 + dy;
                    int px = origin_x * 2 + dx;

                    if (!(0 <= py && py <= (height - 1) * 2 && 0 <= px && px <= (width - 1) * 2)) {
                        invalid = true;
                        blocker_cand = std::nullopt;
                        break;
                    }
                    if (problem_.color(py >> 1, px >> 1) != problem_.color((py + 1) >> 1, (px + 1) >> 1)) {
                        invalid = true;
                        blocker_cand = std::nullopt;
                        break;
                    }
                    if ((py & 1) == 1) {
                        if (board_.vertical(py >> 1, px >> 1) == BoardManager::Border::kWall) {
                            invalid = true;
                            blocker_cand = Glucose::mkLit(board_.VerticalVar(py >> 1, px >> 1), false);
                        }
                    } else {
                        if (board_.horizontal(py >> 1, px >> 1) == BoardManager::Border::kWall) {
                            invalid = true;
                            blocker_cand = Glucose::mkLit(board_.HorizontalVar(py >> 1, px >> 1), false);
                        }
                    }
                }

                if (!invalid) {
                    found = true;
                    break;
                }
                if (blocker_cand) {
                    blockers.insert(*blocker_cand);
                }
            }

            if (found) {
                break;
            }
        }

        if (!found) {
            // TODO: compute more refined reason
            std::set<Glucose::Lit> reason = blockers;
            {
                auto app = board_.ReasonForUnit(info, i);
                reason.insert(app.begin(), app.end());
            }
            {
                auto app = board_.ReasonForPotentialUnitBoundary(info, potential_unit_id);
                reason.insert(app.begin(), app.end());
            }
            for (int g : adjacent_potential_units[potential_unit_id]) {
                auto app = board_.ReasonForPotentialUnitBoundary(info, g);
                reason.insert(app.begin(), app.end());
            }
            for (auto [y, x] : info.potential_units.group(potential_unit_id)) {
                if (y > 0 && board_.vertical(y - 1, x) == BoardManager::Border::kWall &&
                    problem_.color(y, x) != problem_.color(y - 1, x) &&
                    adjacent_potential_units_set.count({potential_unit_id, info.potential_units.group_id(y - 1, x)}) ==
                        0) {
                    reason.insert(Glucose::mkLit(board_.VerticalVar(y - 1, x)));
                }
                if (y < height - 1 && board_.vertical(y, x) == BoardManager::Border::kWall &&
                    problem_.color(y, x) != problem_.color(y + 1, x) &&
                    adjacent_potential_units_set.count({potential_unit_id, info.potential_units.group_id(y + 1, x)}) ==
                        0) {
                    reason.insert(Glucose::mkLit(board_.VerticalVar(y, x)));
                }
                if (x > 0 && board_.horizontal(y, x - 1) == BoardManager::Border::kWall &&
                    problem_.color(y, x) != problem_.color(y, x - 1) &&
                    adjacent_potential_units_set.count({potential_unit_id, info.potential_units.group_id(y, x - 1)}) ==
                        0) {
                    reason.insert(Glucose::mkLit(board_.HorizontalVar(y, x - 1)));
                }
                if (x < width - 1 && board_.horizontal(y, x) == BoardManager::Border::kWall &&
                    problem_.color(y, x) != problem_.color(y, x + 1) &&
                    adjacent_potential_units_set.count({potential_unit_id, info.potential_units.group_id(y, x + 1)}) ==
                        0) {
                    reason.insert(Glucose::mkLit(board_.HorizontalVar(y, x)));
                }
            }

            return std::vector<Glucose::Lit>(reason.begin(), reason.end());
        }
    }

    return std::nullopt;
}
