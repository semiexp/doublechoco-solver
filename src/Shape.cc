#include "Shape.h"

#include <cassert>

namespace doublechoco {

void Shape::clear() {
    cells.clear();
    connections.clear();
}

void Shape::Rotate90To(Shape& dest) const {
    dest.clear();
    for (auto& [y, x] : cells) {
        dest.cells.push_back({x, -y});
    }
    for (auto& [y, x] : connections) {
        dest.connections.push_back({x, -y});
    }
    std::sort(dest.cells.begin(), dest.cells.end());
    dest.Normalize();
}

void Shape::Rotate180To(Shape& dest) const {
    dest.clear();
    for (auto& [y, x] : cells) {
        dest.cells.push_back({-y, -x});
    }
    std::reverse(dest.cells.begin(), dest.cells.end());
    for (auto& [y, x] : connections) {
        dest.connections.push_back({-y, -x});
    }
    dest.Normalize();
}

void Shape::FlipYTo(Shape& dest) const {
    dest.clear();
    int p = cells.size();
    while (p > 0) {
        int q = p - 1;
        while (q > 0 && cells[q].first == cells[q - 1].first)
            --q;
        for (int i = q; i < p; ++i) {
            dest.cells.push_back({-cells[i].first, cells[i].second});
        }
        p = q;
    }
    for (auto& [y, x] : connections) {
        dest.connections.push_back({-y, x});
    }
    dest.Normalize();
}

void Shape::Normalize() {
    assert(!cells.empty());
    auto min_pos = cells[0];
    for (auto& c : cells) {
        c.first -= min_pos.first;
        c.second -= min_pos.second;
    }
    for (auto& c : connections) {
        c.first -= (min_pos.first << 1);
        c.second -= (min_pos.second << 1);
    }
}

bool Shape::operator==(const Shape& rhs) const { return cells == rhs.cells; }

}
