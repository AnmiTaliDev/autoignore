#include "Detector.hpp"

#include <algorithm>
#include <fnmatch.h>
#include <unordered_set>

namespace fs = std::filesystem;

Detector::Detector(TemplateStore& store) : store(store) {}

bool Detector::pattern_matches(const std::string& name, const std::string& pattern) {
    return fnmatch(pattern.c_str(), name.c_str(), FNM_CASEFOLD) == 0;
}

std::vector<std::string> Detector::detect(const fs::path& dir) {
    std::unordered_set<std::string> filenames;
    std::unordered_set<std::string> extensions;

    auto collect = [&](const fs::path& p) {
        auto fname = p.filename().string();
        if (!fname.empty() && fname[0] != '.') {
            filenames.insert(fname);
            if (p.has_extension()) extensions.insert(p.extension().string());
        }
    };

    try {
        for (const auto& e : fs::directory_iterator(dir)) collect(e.path());
        for (auto it = fs::recursive_directory_iterator(dir); it != fs::recursive_directory_iterator{}; ++it) {
            if (it.depth() > 3) { it.disable_recursion_pending(); continue; }
            collect(it->path());
        }
    } catch (...) {}

    std::unordered_set<std::string> suggested;

    for (const auto& tmpl : store.all()) {
        if (tmpl.detect_patterns.empty()) continue;
        for (const auto& pattern : tmpl.detect_patterns) {
            bool matched = false;
            for (const auto& fname : filenames) {
                if (pattern_matches(fname, pattern)) { matched = true; break; }
            }
            if (!matched) {
                for (const auto& ext : extensions) {
                    if (pattern_matches("x" + ext, pattern)) { matched = true; break; }
                }
            }
            if (matched) { suggested.insert(tmpl.name); break; }
        }
    }

    std::vector<std::string> result(suggested.begin(), suggested.end());
    std::sort(result.begin(), result.end());
    return result;
}
