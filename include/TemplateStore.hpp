#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class TemplateStore {
public:
    struct Template {
        std::string name;
        fs::path path;
        std::vector<std::string> detect_patterns;
    };

    TemplateStore();

    const std::vector<Template>& all();
    const Template* find(const std::string& name);
    std::vector<const Template*> search(const std::string& query);
    std::string read_content(const Template& t) const;
    const std::vector<fs::path>& paths() const;

private:
    std::vector<fs::path> search_paths;
    std::vector<Template> cache;
    bool cache_valid = false;

    void init_paths();
    std::vector<std::string> parse_detect_patterns(const fs::path& path);
};
