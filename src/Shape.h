#pragma once

#include <algorithm>
#include <vector>

namespace doublechoco {

struct Shape {
    std::vector<std::pair<int, int>> cells; // invariant: `cells` must be sorted and (0, 0) must be in `cells`
    std::vector<std::pair<int, int>> connections;

    void clear();
    void Rotate90To(Shape& dest) const;
    void Rotate180To(Shape& dest) const;
    void FlipYTo(Shape& dest) const;

    void Normalize();

    bool operator==(const Shape& rhs) const;
};

}
