#pragma once

#include <optional>
#include <vector>

#include "Grid.h"
#include "evolmino/Problem.h"

namespace evolmino {

enum EvolminoAnswerCell {
    kUndecided,
    kSquare,
    kEmpty,
};

using EvolminoAnswer = Grid<EvolminoAnswerCell>;

std::optional<EvolminoAnswer> FindAnswer(const Problem& problem);
std::optional<EvolminoAnswer> Solve(const Problem& problem);

}
