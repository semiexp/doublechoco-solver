#pragma once

#include <algorithm>
#include <cassert>

template <typename T> class Grid {
public:
    Grid(int height, int width, const T& initial) : height_(height), width_(width) {
        data_ = new T[height * width];
        std::fill(data_, data_ + height * width, initial);
    }
    Grid(const Grid<T>& other) : height_(other.height_), width_(other.width_) {
        data_ = new T[height_ * width_];
        std::copy(other.data_, other.data_ + height_ * width_, data_);
    }

    ~Grid() { delete[] data_; }

    int height() const { return height_; }
    int width() const { return width_; }

    T& at(int y, int x) {
        assert(0 <= y && y < height_ && 0 <= x && x < width_);
        return data_[y * width_ + x];
    }

    const T& at(int y, int x) const {
        assert(0 <= y && y < height_ && 0 <= x && x < width_);
        return data_[y * width_ + x];
    }

    T& at(const std::pair<int, int>& p) {
        return at(p.first, p.second);
    }

    const T& at(const std::pair<int, int>& p) const {
        return at(p.first, p.second);
    }

private:
    int height_, width_;
    T* data_;
};
