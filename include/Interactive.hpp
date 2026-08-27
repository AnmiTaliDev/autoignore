#pragma once

#include <string>
#include <unordered_set>
#include <vector>

class InteractiveSelector {
public:
    std::vector<std::string> select(
        const std::vector<std::string>& all_names,
        const std::unordered_set<std::string>& preselected = {});

private:
    enum Key { K_UP = 1000, K_DOWN, K_ENTER, K_SPACE, K_QUIT, K_BACKSPACE };

    int read_key();
    void move_up_and_clear(int lines);
};
