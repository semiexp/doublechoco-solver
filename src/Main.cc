#include "Problem.h"
#include "Solver.h"

#include <cstdlib>
#include <string>

int main(int argc, char** argv) {
    std::optional<Problem> problem_opt = Problem::ParseURL(argv[1]);
    if (!problem_opt) {
        printf("Error: invalid url\n");
        return 0;
    }
    Problem problem = *problem_opt;
    int height = problem.height();
    int width = problem.width();

    std::optional<DoublechocoAnswer> ans = Solve(problem);
    if (!ans) {
        printf("No answer\n");
        return 0;
    }
    assert(ans.has_value());

    for (int y = -1; y < height * 2; ++y) {
        for (int x = -1; x < width * 2; ++x) {
            if ((y & 1) == 0 && (x & 1) == 0) {
                printf(" ");
            } else if ((y & 1) == 1 && (x & 1) == 1) {
                printf("+");
            } else if ((y & 1) == 1 && (x & 1) == 0) {
                if (y == -1 || y == height * 2 - 1) {
                    printf("-");
                    continue;
                }
                switch (ans->vertical[y / 2][x / 2]) {
                case DoublechocoAnswer::Border::kUndecided:
                    printf("?");
                    break;
                case DoublechocoAnswer::Border::kWall:
                    printf("-");
                    break;
                case DoublechocoAnswer::Border::kConnected:
                    printf(" ");
                    break;
                }
            } else {
                if (x == -1 || x == width * 2 - 1) {
                    printf("|");
                    continue;
                }
                switch (ans->horizontal[y / 2][x / 2]) {
                case DoublechocoAnswer::Border::kUndecided:
                    printf("?");
                    break;
                case DoublechocoAnswer::Border::kWall:
                    printf("|");
                    break;
                case DoublechocoAnswer::Border::kConnected:
                    printf(" ");
                    break;
                }
            }
        }
        printf("\n");
    }
    printf("\n");

    return 0;
}
