#pragma once

#include "TemplateStore.hpp"
#include "Config.hpp"

#include <filesystem>
#include <string>
#include <vector>

class Detector {
public:
    explicit Detector(TemplateStore& store, int max_depth = AUTOIGNORE_DEFAULT_MAX_DEPTH);

    std::vector<std::string> detect(const std::filesystem::path& dir);

private:
    TemplateStore& store;
    int max_depth;
};
