#pragma once

#include "Grid.h"

#include <vector>

template <typename T> class Group {
public:
    using iterator = typename std::vector<T>::const_iterator;

    Group(const iterator start, const iterator end) : start_(start), end_(end) {}

    const T& operator[](size_t idx) const { return *(start_ + idx); }
    iterator begin() const { return start_; }
    iterator end() const { return end_; }
    int size() const { return std::distance(start_, end_); }

private:
    const iterator start_, end_;
};

class GroupInfo {
public:
    GroupInfo(Grid<int>&& group_id);

    int group_id(int y, int x) const { return group_id_.at(y, x); }
    int num_groups() const { return groups_offset_.size() - 1; }
    const Group<std::pair<int, int>> group(int id) const {
        return Group<std::pair<int, int>>(groups_raw_.begin() + groups_offset_[id],
                                          groups_raw_.begin() + groups_offset_[id + 1]);
    };

private:
    Grid<int> group_id_;
    std::vector<std::pair<int, int>> groups_raw_;
    std::vector<int> groups_offset_;
};
