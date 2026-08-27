#pragma once

#include "TemplateStore.hpp"

#include <filesystem>
#include <string>
#include <vector>

class Detector {
public:
    explicit Detector(TemplateStore& store);

    std::vector<std::string> detect(const std::filesystem::path& dir);

private:
    TemplateStore& store;
};
