#include "Problem.h"
#include "Solver.h"

int main() {
    // Random example to check if the solver is capable of finding a valid answer.
    // Possible division:
    // 112233
    // 411223
    // 444223
    // 554422
    // 544466
    // 547777
    const int height = 6;
    const int width = 6;
    int base[height][width] = {
        {1, 1, 1, 1, 0, 0}, {1, 0, 0, 1, 1, 1}, {1, 1, 1, 0, 0, 1},
        {1, 1, 1, 0, 0, 0}, {0, 0, 0, 0, 0, 1}, {0, 0, 0, 0, 1, 1},
    };

    Problem problem(height, width);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            problem.setColor(y, x, base[y][x]);
        }
    }

    std::optional<DoublechocoAnswer> ans = FindAnswer(problem);
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
