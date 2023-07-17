#include "evolmino/Problem.h"
#include "evolmino/Solver.h"

using namespace evolmino;

int main(int argc, char** argv) {
    std::optional<Problem> problem_opt = Problem::ParseURL(argv[1]);

    if (!problem_opt) {
        printf("Error: invalid url\n");
        return 0;
    }

    Problem problem = *problem_opt;
    int height = problem.height();
    int width = problem.width();
    std::optional<EvolminoAnswer> ans = Solve(problem);
    if (!ans) {
        puts("No answer");
        return 0;
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            switch (ans->at(y, x)) {
                case EvolminoAnswerCell::kSquare:
                    printf("# ");
                    break;
                case EvolminoAnswerCell::kEmpty:
                    printf("x ");
                    break;
                default:
                    printf(". ");
                    break;
            }
        }
        puts("");
    }
    return 0;
}
