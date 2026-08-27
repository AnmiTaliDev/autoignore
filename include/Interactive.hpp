#pragma once

#include "Config.hpp"

#include <string>
#include <unordered_set>
#include <vector>

class InteractiveSelector {
public:
    explicit InteractiveSelector(int page_size = AUTOIGNORE_DEFAULT_PAGE_SIZE);

    std::vector<std::string> select(
        const std::vector<std::string>& all_names,
        const std::unordered_set<std::string>& preselected = {});

private:
    int page_size;

    enum Key {
        K_NONE = 0,
        K_UP = 1000,
        K_DOWN,
        K_PAGE_UP,
        K_PAGE_DOWN,
        K_HOME,
        K_END,
        K_ENTER,
        K_SPACE,
        K_TAB,
        K_BACKSPACE,
        K_DELETE,
        K_CTRL_A,
        K_CTRL_U,
        K_CTRL_W,
        K_CTRL_R,
        K_QUIT
    };

    int read_key();
    void move_up_and_clear(int lines);
};
