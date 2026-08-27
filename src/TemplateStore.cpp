#include "TemplateStore.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <cstdlib>
#include <unistd.h>

TemplateStore::TemplateStore() {
    init_paths();
}

static fs::path get_executable_dir() {
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len != -1) {
        buf[len] = '\0';
        return fs::path(buf).parent_path();
    }
    return {};
}

void TemplateStore::init_paths() {
    auto add_path = [this](const fs::path& p) {
        if (p.empty()) return;
        std::error_code ec;
        fs::path norm = fs::weakly_canonical(p, ec);
        if (ec) norm = fs::absolute(p, ec);
        for (const auto& existing : search_paths) {
            std::error_code ec2;
            fs::path existing_norm = fs::weakly_canonical(existing, ec2);
            if (!ec && !ec2 && existing_norm == norm) return;
            if (existing == p) return;
        }
        search_paths.push_back(p);
    };

    if (const char* custom_path = std::getenv("AUTOIGNORE_PATH")) {
        std::string_view sv(custom_path);
        size_t start = 0;
        while (start < sv.size()) {
            size_t end = sv.find(':', start);
            if (end == std::string_view::npos) end = sv.size();
            if (end > start) {
                add_path(fs::path(std::string(sv.substr(start, end - start))));
            }
            start = end + 1;
        }
    }

    std::error_code cwd_ec;
    fs::path cwd = fs::current_path(cwd_ec);
    if (!cwd_ec) {
        add_path(cwd / "template");
    }

    fs::path exe_dir = get_executable_dir();
    if (!exe_dir.empty()) {
        add_path(exe_dir / "template");
        add_path(exe_dir / ".." / "share" / "autoignore" / "template");
    }

    if (const char* xdg_data = std::getenv("XDG_DATA_HOME")) {
        if (xdg_data[0] != '\0') {
            add_path(fs::path(xdg_data) / "autoignore" / "template");
        }
    } else if (const char* home = std::getenv("HOME")) {
        add_path(fs::path(home) / ".local" / "share" / "autoignore" / "template");
    }

    if (const char* xdg_dirs = std::getenv("XDG_DATA_DIRS")) {
        std::string_view sv(xdg_dirs);
        size_t start = 0;
        while (start < sv.size()) {
            size_t end = sv.find(':', start);
            if (end == std::string_view::npos) end = sv.size();
            if (end > start) {
                add_path(fs::path(std::string(sv.substr(start, end - start))) / "autoignore" / "template");
            }
            start = end + 1;
        }
    } else {
        add_path("/usr/local/share/autoignore/template");
        add_path("/usr/share/autoignore/template");
    }
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

std::vector<std::string> TemplateStore::parse_exclude_dirs(const fs::path& path) {
    std::ifstream f(path);
    std::vector<std::string> dirs;
    std::string line;
    bool in_header = true;
    while (std::getline(f, line)) {
        if (in_header) {
            if (line.empty()) { in_header = false; continue; }
            if (line[0] == '#') continue;
            in_header = false;
        }
        if (line.empty() || line[0] == '#' || line[0] == '!' || line[0] == '/') continue;
        if (line.back() != '/') continue;
        std::string dirname = line.substr(0, line.size() - 1);
        if (dirname.find('/') != std::string::npos) continue;
        if (dirname.find_first_of("*?[") != std::string::npos) continue;
        if (dirname.empty() || dirname[0] == '.') continue;
        dirs.push_back(dirname);
    }
    return dirs;
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
                t.exclude_dirs = parse_exclude_dirs(entry.path());
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
        if (n.find(q) != std::string::npos) results.push_back(&t);\
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
