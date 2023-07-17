#include "Group.h"

GroupInfo::GroupInfo(Grid<int>&& group_id) : group_id_(group_id) {
    int max_group_id = 0;
    for (int y = 0; y < group_id_.height(); ++y) {
        for (int x = 0; x < group_id_.width(); ++x) {
            max_group_id = std::max(max_group_id, group_id_.at(y, x));
        }
    }

    groups_offset_ = std::vector<int>(max_group_id + 2, 0);
    for (int y = 0; y < group_id_.height(); ++y) {
        for (int x = 0; x < group_id_.width(); ++x) {
            int id = group_id_.at(y, x);
            if (id >= 0) {
                ++groups_offset_[id + 1];
            }
        }
    }
    for (int i = 1; i < groups_offset_.size(); ++i) {
        groups_offset_[i] += groups_offset_[i - 1];
    }
    std::vector<int> next_pos = groups_offset_;
    groups_raw_ = std::vector<std::pair<int, int>>(group_id_.height() * group_id_.width());
    for (int y = 0; y < group_id_.height(); ++y) {
        for (int x = 0; x < group_id_.width(); ++x) {
            int id = group_id_.at(y, x);
            if (id >= 0) {
                groups_raw_[next_pos[id]++] = std::make_pair(y, x);
            }
        }
    }
}
