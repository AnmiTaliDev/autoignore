#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

class TemplateStore {
public:
    struct Template {
        std::string name;
        fs::path path;
        std::vector<std::string> detect_patterns;
        std::vector<std::string> exclude_dirs;
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
    std::unordered_map<std::string, size_t> name_to_index;
    bool cache_valid = false;

    void init_paths();
    void parse_template_header(const fs::path& path,
                               std::vector<std::string>& detect_patterns,
                               std::vector<std::string>& exclude_dirs);
};
