#include "Detector.hpp"

#include <algorithm>
#include <fnmatch.h>
#include <unordered_set>

namespace fs = std::filesystem;

Detector::Detector(TemplateStore& store) : store(store) {}

static inline std::string to_lower_copy(std::string_view str) {
    std::string s(str);
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::vector<std::string> Detector::detect(const fs::path& dir) {
    std::unordered_set<std::string> excluded_dirs;
    for (const auto& tmpl : store.all()) {
        for (const auto& d : tmpl.exclude_dirs)
            excluded_dirs.insert(d);
    }

    std::unordered_set<std::string> filenames_lower;
    std::unordered_set<std::string> extensions_lower;
    std::vector<std::string> all_filenames_lower;

    filenames_lower.reserve(256);
    extensions_lower.reserve(64);
    all_filenames_lower.reserve(256);

    std::error_code ec;
    auto iter_options = fs::directory_options::skip_permission_denied;
    fs::recursive_directory_iterator it(dir, iter_options, ec);
    fs::recursive_directory_iterator end;

    while (!ec && it != end) {
        const auto& entry = *it;
        auto fname = entry.path().filename().string();

        if (fname.empty()) {
            it.increment(ec);
            continue;
        }

        if (fname[0] == '.') {
            if (entry.is_directory(ec)) it.disable_recursion_pending();
            it.increment(ec);
            continue;
        }

        if (entry.is_directory(ec)) {
            if (excluded_dirs.find(fname) != excluded_dirs.end()) {
                it.disable_recursion_pending();
                it.increment(ec);
                continue;
            }
            if (it.depth() > 3) {
                it.disable_recursion_pending();
                it.increment(ec);
                continue;
            }
        } else if (entry.is_regular_file(ec)) {
            std::string fname_lower = to_lower_copy(fname);
            if (filenames_lower.insert(fname_lower).second) {
                all_filenames_lower.push_back(fname_lower);
            }

            if (entry.path().has_extension()) {
                std::string ext_lower = to_lower_copy(entry.path().extension().string());
                extensions_lower.insert(std::move(ext_lower));
            }
        }

        it.increment(ec);
    }

    std::unordered_set<std::string> suggested;
    suggested.reserve(16);

    for (const auto& tmpl : store.all()) {
        if (tmpl.detect_patterns.empty()) continue;

        for (const auto& pattern : tmpl.detect_patterns) {
            std::string pat_lower = to_lower_copy(pattern);
            bool matched = false;

            if (pat_lower.rfind("*.", 0) == 0 && pat_lower.find_first_of("*?[", 2) == std::string::npos) {
                std::string target_ext = pat_lower.substr(1);
                matched = (extensions_lower.find(target_ext) != extensions_lower.end());
            } else if (pat_lower.find_first_of("*?[") == std::string::npos) {
                matched = (filenames_lower.find(pat_lower) != filenames_lower.end());
            } else {
                for (const auto& fname_lower : all_filenames_lower) {
                    if (fnmatch(pat_lower.c_str(), fname_lower.c_str(), 0) == 0) {
                        matched = true;
                        break;
                    }
                }
            }

            if (matched) {
                suggested.insert(tmpl.name);
                break;
            }
        }
    }

    std::vector<std::string> result(suggested.begin(), suggested.end());
    std::sort(result.begin(), result.end());
    return result;
}
