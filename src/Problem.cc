#include "Problem.h"

Problem::Problem(int height, int width)
    : height_(height), width_(width), color_(height * width, -1), num_(height * width, -1) {}
