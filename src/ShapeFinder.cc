#include "ShapeFinder.h"

#include <algorithm>
#include <limits>
#include <set>

#include "Grid.h"

ShapeFinder::ShapeFinder(const Problem& problem, Glucose::Var origin)
    : problem_(problem), board_(problem.height(), problem.width(), origin) {}

bool ShapeFinder::initialize(Glucose::Solver& solver) {
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

namespace {

void SetGroupId(Grid<int>& group_id, const Problem& problem, const BoardManager& board, int y, int x, int color,
                int id) {
    if (problem.color(y, x) != color) {
        return;
    }
    if (group_id.at(y, x) != -1) {
        return;
    }
    group_id.at(y, x) = id;

    if (y > 0 && board.vertical(y - 1, x) != BoardManager::Border::kWall) {
        SetGroupId(group_id, problem, board, y - 1, x, color, id);
    }
    if (y < group_id.height() - 1 && board.vertical(y, x) != BoardManager::BoardManager::kWall) {
        SetGroupId(group_id, problem, board, y + 1, x, color, id);
    }
    if (x > 0 && board.horizontal(y, x - 1) != BoardManager::Border::kWall) {
        SetGroupId(group_id, problem, board, y, x - 1, color, id);
    }
    if (x < group_id.width() - 1 && board.horizontal(y, x) != BoardManager::Border::kWall) {
        SetGroupId(group_id, problem, board, y, x + 1, color, id);
    }
}

void GetConnectedGroup(Grid<bool>& visited, const Problem& problem, const BoardManager& board, int y, int x, int color,
                       std::vector<std::pair<int, int>>& group, std::vector<std::pair<int, int>>& connections) {
    if (problem.color(y, x) != color) {
        return;
    }
    if (visited.at(y, x)) {
        return;
    }
    visited.at(y, x) = true;
    group.push_back({y, x});

    if (y > 0 && board.vertical(y - 1, x) == BoardManager::Border::kConnected && problem.color(y - 1, x) == color) {
        connections.push_back({y * 2 - 1, x * 2});
        GetConnectedGroup(visited, problem, board, y - 1, x, color, group, connections);
    }
    if (y < visited.height() - 1 && board.vertical(y, x) == BoardManager::Border::kConnected &&
        problem.color(y + 1, x) == color) {
        connections.push_back({y * 2 + 1, x * 2});
        GetConnectedGroup(visited, problem, board, y + 1, x, color, group, connections);
    }
    if (x > 0 && board.horizontal(y, x - 1) == BoardManager::Border::kConnected && problem.color(y, x - 1) == color) {
        connections.push_back({y * 2, x * 2 - 1});
        GetConnectedGroup(visited, problem, board, y, x - 1, color, group, connections);
    }
    if (x < visited.width() - 1 && board.horizontal(y, x) == BoardManager::Border::kConnected &&
        problem.color(y, x + 1) == color) {
        connections.push_back({y * 2, x * 2 + 1});
        GetConnectedGroup(visited, problem, board, y, x + 1, color, group, connections);
    }
}

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

} // namespace

bool ShapeFinder::propagate(Glucose::Solver& solver, Glucose::Lit p) {
    solver.registerUndo(var(p), this);
    board_.Decide(p);
    reasons_.push_back({});

    int height = problem_.height();
    int width = problem_.width();

    Grid<int> group_id(height, width, -1);
    int gid_last = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (group_id.at(y, x) != -1) {
                continue;
            }
            SetGroupId(group_id, problem_, board_, y, x, problem_.color(y, x), gid_last++);
        }
    }
    std::vector<std::vector<std::pair<int, int>>> groups(gid_last);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            groups[group_id.at(y, x)].push_back({y, x});
        }
    }
    std::vector<std::pair<int, int>> adjacent_groups;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (y < height - 1 && problem_.color(y, x) != problem_.color(y + 1, x) &&
                board_.vertical(y, x) != BoardManager::Border::kWall) {
                adjacent_groups.push_back({group_id.at(y, x), group_id.at(y + 1, x)});
                adjacent_groups.push_back({group_id.at(y + 1, x), group_id.at(y, x)});
            }
            if (x < width - 1 && problem_.color(y, x) != problem_.color(y, x + 1) &&
                board_.horizontal(y, x) != BoardManager::Border::kWall) {
                adjacent_groups.push_back({group_id.at(y, x), group_id.at(y, x + 1)});
                adjacent_groups.push_back({group_id.at(y, x + 1), group_id.at(y, x)});
            }
        }
    }
    std::sort(adjacent_groups.begin(), adjacent_groups.end());
    adjacent_groups.erase(std::unique(adjacent_groups.begin(), adjacent_groups.end()), adjacent_groups.end());

    auto is_connectable = [&](int y, int x) {
        if (!(0 <= y && y < height * 2 - 1 && 0 <= x && x < width * 2 - 1)) {
            return false;
        }
        if (problem_.color(y >> 1, x >> 1) != problem_.color((y + 1) >> 1, (x + 1) >> 1)) {
            return false;
        }

        assert(y % 2 != x % 2);
        if (y % 2 == 1) {
            return board_.vertical(y >> 1, x >> 1) != BoardManager::Border::kWall;
        } else {
            return board_.horizontal(y >> 1, x >> 1) != BoardManager::Border::kWall;
        }
    };

    Grid<bool> visited(height, width, false);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (visited.at(y, x))
                continue;
            std::vector<std::pair<int, int>> adj_candidates;

            int gid = group_id.at(y, x);
            for (auto it = std::lower_bound(adjacent_groups.begin(), adjacent_groups.end(),
                                            std::make_pair(group_id.at(y, x), -1));
                 it != adjacent_groups.end() && it->first == gid; ++it) {
                for (auto& p : groups[it->second]) {
                    adj_candidates.push_back(p);
                }
            }

            std::vector<std::pair<int, int>> group, connections;
            // TODO: `group` may be omitted
            GetConnectedGroup(visited, problem_, board_, y, x, problem_.color(y, x), group, connections);

            std::set<std::vector<std::pair<int, int>>> unique_groups;
            bool found_ok = false;
            for (int t = 0; t < 8; ++t) {
                auto [group_trans, connections_trans] =
                    Transform(group, connections, (t >> 2) * 2 - 1, ((t >> 1) & 1) * 2 - 1, t & 1);
                if (!unique_groups.emplace(group_trans).second) {
                    continue;
                }

                for (auto [y2, x2] : adj_candidates) {
                    assert(problem_.color(y, x) != problem_.color(y2, x2));
                    bool flg = true;
                    for (auto [dy, dx] : connections_trans) {
                        if (!is_connectable(y2 * 2 + dy, x2 * 2 + dx)) {
                            flg = false;
                            break;
                        }
                    }
                    if (flg) {
                        found_ok = true;
                    }
                }
                if (found_ok) {
                    break;
                }
            }

            if (!found_ok) {
                std::set<std::pair<int, int>> blockers;
                unique_groups.clear();

                for (int t = 0; t < 8; ++t) {
                    auto [group_trans, connections_trans] =
                        Transform(group, connections, (t >> 2) * 2 - 1, ((t >> 1) & 1) * 2 - 1, t & 1);
                    if (!unique_groups.emplace(group_trans).second) {
                        continue;
                    }

                    for (auto [y2, x2] : adj_candidates) {
                        assert(problem_.color(y, x) != problem_.color(y2, x2));

                        bool skip = false;
                        std::vector<std::pair<int, int>> blocker_cand;
                        for (auto [dy, dx] : connections_trans) {
                            int y3 = y2 * 2 + dy, x3 = x2 * 2 + dx;
                            if (!(0 <= y3 && y3 < height * 2 - 1 && 0 <= x3 && x3 < width * 2 - 1)) {
                                skip = true;
                                break;
                            }
                            if (problem_.color(y3 >> 1, x3 >> 1) != problem_.color((y3 + 1) >> 1, (x3 + 1) >> 1)) {
                                skip = true;
                                break;
                            }
                            if (!is_connectable(y3, x3)) {
                                if (blockers.count({y3, x3})) {
                                    skip = true;
                                    break;
                                }
                                blocker_cand.push_back({y3, x3});
                            }
                        }
                        if (skip) {
                            continue;
                        }
                        assert(!blocker_cand.empty());
                        blockers.insert(blocker_cand[0]);
                    }
                    if (found_ok) {
                        break;
                    }
                }

                std::vector<Glucose::Lit> reason;

                for (auto& [y2, x2] : blockers) {
                    if (0 <= y2 && y2 < height * 2 - 1 && 0 <= x2 && x2 < width * 2 - 1) {
                        if (problem_.color(y2 >> 1, x2 >> 1) != problem_.color((y2 + 1) >> 1, (x2 + 1) >> 1)) {
                            continue;
                        }
                        if (y2 % 2 == 1) {
                            reason.push_back(Glucose::mkLit(board_.VerticalVar(y2 >> 1, x2 >> 1)));
                        } else {
                            reason.push_back(Glucose::mkLit(board_.HorizontalVar(y2 >> 1, x2 >> 1)));
                        }
                    }
                }
                for (auto& [y2, x2] : group) {
                    if (y2 > 0 && board_.vertical(y2 - 1, x2) == BoardManager::Border::kConnected &&
                        problem_.color(y2 - 1, x2) == problem_.color(y2, x2)) {
                        reason.push_back(Glucose::mkLit(board_.VerticalVar(y2 - 1, x2), true));
                    }
                    if (y2 < height - 1 && board_.vertical(y2, x2) == BoardManager::Border::kConnected &&
                        problem_.color(y2 + 1, x2) == problem_.color(y2, x2)) {
                        reason.push_back(Glucose::mkLit(board_.VerticalVar(y2, x2), true));
                    }
                    if (x2 > 0 && board_.horizontal(y2, x2 - 1) == BoardManager::Border::kConnected &&
                        problem_.color(y2, x2 - 1) == problem_.color(y2, x2)) {
                        reason.push_back(Glucose::mkLit(board_.HorizontalVar(y2, x2 - 1), true));
                    }
                    if (x2 < width - 1 && board_.horizontal(y2, x2) == BoardManager::Border::kConnected &&
                        problem_.color(y2, x2 + 1) == problem_.color(y2, x2)) {
                        reason.push_back(Glucose::mkLit(board_.HorizontalVar(y2, x2), true));
                    }
                }
                std::vector<int> related_groups;
                related_groups.push_back(group_id.at(y, x));
                for (auto it = std::lower_bound(adjacent_groups.begin(), adjacent_groups.end(),
                                                std::make_pair(group_id.at(y, x), -1));
                     it != adjacent_groups.end() && it->first == gid; ++it) {
                    related_groups.push_back(it->second);
                }
                for (int g : related_groups) {
                    for (auto& [y2, x2] : groups[g]) {
                        if (y2 > 0 && board_.vertical(y2 - 1, x2) == BoardManager::Border::kWall) {
                            reason.push_back(Glucose::mkLit(board_.VerticalVar(y2 - 1, x2)));
                        }
                        if (y2 < height - 1 && board_.vertical(y2, x2) == BoardManager::Border::kWall) {
                            reason.push_back(Glucose::mkLit(board_.VerticalVar(y2, x2)));
                        }
                        if (x2 > 0 && board_.horizontal(y2, x2 - 1) == BoardManager::Border::kWall) {
                            reason.push_back(Glucose::mkLit(board_.HorizontalVar(y2, x2 - 1)));
                        }
                        if (x2 < width - 1 && board_.horizontal(y2, x2) == BoardManager::Border::kWall) {
                            reason.push_back(Glucose::mkLit(board_.HorizontalVar(y2, x2)));
                        }
                    }
                }

                std::sort(reason.begin(), reason.end());
                reason.erase(std::unique(reason.begin(), reason.end()), reason.end());
                reasons_.back() = reason;
                return false;
            }
        }
    }

    return true;
}

void ShapeFinder::calcReason(Glucose::Solver& solver, Glucose::Lit p, Glucose::Lit extra,
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

void ShapeFinder::undo(Glucose::Solver& solver, Glucose::Lit p) {
    board_.Undo(p);
    reasons_.pop_back();
}
