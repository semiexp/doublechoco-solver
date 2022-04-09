#pragma once

#include "Problem.h"

#include <optional>
#include <vector>

struct DoublechocoAnswer {
    enum Border {
        kUndecided,
        kWall,
        kConnected,
    };

    std::vector<std::vector<Border>> horizontal, vertical;
};

std::optional<DoublechocoAnswer> FindAnswer(const Problem& problem);
std::optional<DoublechocoAnswer> Solve(const Problem& problem);
