#include "TemplateStore.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cstdlib>

TemplateStore::TemplateStore() {
    init_paths();
}

void TemplateStore::init_paths() {
    if (const char* home = std::getenv("HOME")) {
        search_paths.push_back(fs::path(home) / ".local/share/autoignore/template");
    }
    search_paths.push_back("/usr/local/share/autoignore/template");
    search_paths.push_back("/usr/share/autoignore/template");
}

std::vector<std::string> TemplateStore::parse_detect_patterns(const fs::path& path) {
    std::ifstream f(path);
    std::vector<std::string> patterns;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) break;
        if (line.rfind("# @detect:", 0) == 0) {
            std::istringstream ss(line.substr(10));
            std::string token;
            while (ss >> token) patterns.push_back(token);
        } else if (line[0] != '#') {
            break;
        }
    }
    return patterns;
}

const std::vector<TemplateStore::Template>& TemplateStore::all() {
    if (cache_valid) return cache;
    cache.clear();
    std::unordered_map<std::string, Template> seen;
    for (const auto& base : search_paths) {
        if (!fs::exists(base) || !fs::is_directory(base)) continue;
        for (const auto& entry : fs::directory_iterator(base)) {
            if (!entry.is_regular_file()) continue;
            auto fname = entry.path().filename().string();
            if (!fname.ends_with(".gitignore")) continue;
            std::string name = fname.substr(0, fname.size() - 10);
            if (!seen.count(name)) {
                Template t;
                t.name = name;
                t.path = entry.path();
                t.detect_patterns = parse_detect_patterns(entry.path());
                seen.emplace(name, std::move(t));
            }
        }
    }
    for (auto& [k, v] : seen) cache.push_back(std::move(v));
    std::sort(cache.begin(), cache.end(),
              [](const Template& a, const Template& b) { return a.name < b.name; });
    cache_valid = true;
    return cache;
}

const TemplateStore::Template* TemplateStore::find(const std::string& name) {
    all();
    for (const auto& t : cache) {
        if (t.name == name) return &t;
    }
    return nullptr;
}

std::vector<const TemplateStore::Template*> TemplateStore::search(const std::string& query) {
    all();
    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);
    std::vector<const Template*> results;
    for (const auto& t : cache) {
        std::string n = t.name;
        std::transform(n.begin(), n.end(), n.begin(), ::tolower);
        if (n.find(q) != std::string::npos) results.push_back(&t);
    }
    return results;
}

std::string TemplateStore::read_content(const Template& t) const {
    std::ifstream f(t.path);
    if (!f) return "";
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

const std::vector<fs::path>& TemplateStore::paths() const {
    return search_paths;
}
