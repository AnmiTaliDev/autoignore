#include "Common.hpp"
#include "Detector.hpp"
#include "Interactive.hpp"
#include "TemplateStore.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include <getopt.h>
#include <unistd.h>

namespace fs = std::filesystem;

static void print_header() {
    std::cout << color::bold << color::cyan << "autoignore" << color::reset
              << " " << color::gray << "2.0.0  gitignore generator" << color::reset << "\n\n";
}

static void print_usage() {
    std::cout
        << color::bold << "Usage:" << color::reset << "\n"
        << "  autoignore [OPTIONS] [TEMPLATES...]\n\n"
        << color::bold << "Options:" << color::reset << "\n"
        << "  -l, --list              List available templates\n"
        << "  -s, --search <query>    Search templates by name\n"
        << "  -i, --interactive       Select templates interactively\n"
        << "  -d, --detect            Auto-detect templates from project files\n"
        << "  -o, --output <file>     Output file (default: .gitignore)\n"
        << "  -a, --append            Append to existing file\n"
        << "  -p, --preview           Preview output without writing\n"
        << "  -u, --dedup             Deduplicate repeated patterns\n"
        << "  -v, --verbose           Verbose output\n"
        << "  -h, --help              Show this help\n\n"
        << color::bold << "Examples:" << color::reset << "\n"
        << "  autoignore cpp cmake\n"
        << "  autoignore --interactive\n"
        << "  autoignore --detect\n"
        << "  autoignore --search py\n"
        << "  autoignore -d -u\n"
        << "  autoignore -d -i\n";
}

static void cmd_list(TemplateStore& store) {
    const auto& templates = store.all();
    if (templates.empty()) {
        std::cout << color::yellow << "No templates found." << color::reset << "\n";
        return;
    }
    std::cout << color::bold << "Available templates (" << templates.size() << "):\n" << color::reset;
    for (const auto& t : templates) {
        std::cout << "  " << color::green << t.name << color::reset;
        if (!t.detect_patterns.empty())
            std::cout << color::gray << "  [auto-detect]" << color::reset;
        std::cout << "\n";
    }
    std::cout << "\n" << color::gray << "Template locations:\n" << color::reset;
    for (const auto& p : store.paths()) {
        std::cout << "  " << color::cyan << p.string() << color::reset;
        if (fs::exists(p) && fs::is_directory(p)) {
            auto n = std::distance(fs::directory_iterator(p), fs::directory_iterator{});
            std::cout << color::gray << "  (" << n << " files)" << color::reset;
        } else {
            std::cout << color::gray << "  (not found)" << color::reset;
        }
        std::cout << "\n";
    }
}

static void cmd_search(TemplateStore& store, const std::string& query) {
    auto results = store.search(query);
    if (results.empty()) {
        std::cout << color::yellow << "No templates matching '" << query << "'.\n" << color::reset;
        return;
    }
    std::cout << color::bold << "Matches for '" << query << "' (" << results.size() << "):\n" << color::reset;
    for (const auto* t : results)
        std::cout << "  " << color::green << t->name << color::reset << "\n";
}

static std::string process_template_content(const std::string& raw_content,
                                            std::unordered_set<std::string>& seen_patterns,
                                            bool dedup)
{
    std::istringstream stream(raw_content);
    std::string line;
    std::string result;
    bool last_was_empty = false;

    while (std::getline(stream, line)) {
        if (line.rfind("# @detect:", 0) == 0) continue;

        std::string trimmed = line;
        size_t first = trimmed.find_first_not_of(" \t\r");
        if (first == std::string::npos) {
            if (!result.empty() && !last_was_empty) {
                result += "\n";
                last_was_empty = true;
            }
            continue;
        }
        size_t last = trimmed.find_last_not_of(" \t\r");
        trimmed = trimmed.substr(first, last - first + 1);

        if (dedup && !trimmed.empty() && trimmed[0] != '#') {
            if (seen_patterns.count(trimmed)) {
                continue;
            }
            seen_patterns.insert(trimmed);
        }

        result += line + "\n";
        last_was_empty = false;
    }

    while (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

static void generate(TemplateStore& store,
                     const std::vector<std::string>& names,
                     const std::string& output,
                     bool append, bool preview, bool dedup, bool verbose)
{
    std::unordered_set<std::string> seen_patterns;

    if (dedup && append && fs::exists(output)) {
        std::ifstream existing_file(output);
        std::string line;
        while (std::getline(existing_file, line)) {
            size_t first = line.find_first_not_of(" \t\r");
            if (first == std::string::npos) continue;
            size_t last = line.find_last_not_of(" \t\r");
            std::string trimmed = line.substr(first, last - first + 1);
            if (!trimmed.empty() && trimmed[0] != '#') {
                seen_patterns.insert(trimmed);
            }
        }
    }

    std::vector<std::pair<std::string, std::string>> contents;
    for (const auto& name : names) {
        const auto* t = store.find(name);
        if (!t) {
            std::cerr << color::yellow << "Warning: template '" << name << "' not found\n" << color::reset;
            continue;
        }
        std::string raw = store.read_content(*t);
        std::string processed = process_template_content(raw, seen_patterns, dedup);
        if (!processed.empty()) {
            contents.emplace_back(name, std::move(processed));
        }
    }

    if (contents.empty()) {
        std::cerr << color::red << "Error: no valid templates\n" << color::reset;
        return;
    }

    if (preview) {
        for (const auto& [name, content] : contents) {
            std::cout << color::bold << color::cyan << "# " << name << color::reset << "\n"
                      << content;
            if (!content.ends_with('\n')) std::cout << "\n";
            std::cout << "\n";
        }
        return;
    }

    std::ofstream f(output, append ? std::ios::app : std::ios::trunc);
    if (!f) {
        std::cerr << color::red << "Error: cannot open " << output << "\n" << color::reset;
        return;
    }

    if (!append || !fs::exists(output) || fs::file_size(output) == 0) {
        f << "# Generated by autoignore\n# Templates:";
        for (const auto& [name, _] : contents) f << " " << name;
        f << "\n\n";
    }

    for (const auto& [name, content] : contents) {
        if (verbose) std::cout << color::green << "  + " << name << color::reset << "\n";
        f << "# " << name << "\n" << content;
        if (!content.ends_with('\n')) f << "\n";
        f << "\n";
    }

    std::cout << color::green << (append ? "Appended to " : "Generated ")
              << color::bold << output << color::reset << "\n";
}

int main(int argc, char* argv[]) {
    color::init();

    bool do_list        = false;
    bool do_interactive = false;
    bool do_detect      = false;
    bool do_preview     = false;
    bool do_dedup       = false;
    bool append         = false;
    bool verbose        = false;
    std::string search_query;
    std::string output = ".gitignore";

    static const struct option long_opts[] = {
        {"list",        no_argument,       nullptr, 'l'},
        {"search",      required_argument, nullptr, 's'},
        {"interactive", no_argument,       nullptr, 'i'},
        {"detect",      no_argument,       nullptr, 'd'},
        {"output",      required_argument, nullptr, 'o'},
        {"append",      no_argument,       nullptr, 'a'},
        {"preview",     no_argument,       nullptr, 'p'},
        {"dedup",       no_argument,       nullptr, 'u'},
        {"verbose",     no_argument,       nullptr, 'v'},
        {"help",        no_argument,       nullptr, 'h'},
        {nullptr, 0, nullptr, 0}
    };

    int c, idx = 0;
    while ((c = getopt_long(argc, argv, "ls:ido:apu vh", long_opts, &idx)) != -1) {
        switch (c) {
            case 'l': do_list = true;           break;
            case 's': search_query = optarg;    break;
            case 'i': do_interactive = true;    break;
            case 'd': do_detect = true;         break;
            case 'o': output = optarg;          break;
            case 'a': append = true;            break;
            case 'p': do_preview = true;        break;
            case 'u': do_dedup = true;          break;
            case 'v': verbose = true;           break;
            case 'h': print_header(); print_usage(); return 0;
            case '?': return 1;
        }
    }

    TemplateStore store;

    if (do_list) {
        print_header();
        cmd_list(store);
        return 0;
    }

    if (!search_query.empty()) {
        print_header();
        cmd_search(store, search_query);
        return 0;
    }

    std::vector<std::string> templates;
    for (int i = optind; i < argc; i++) templates.push_back(argv[i]);

    if (do_detect) {
        Detector detector(store);
        auto detected = detector.detect(".");
        if (detected.empty()) {
            std::cout << color::yellow << "No templates detected for this directory.\n" << color::reset;
        } else {
            std::cout << color::bold << "Detected: " << color::reset;
            for (const auto& t : detected) std::cout << color::green << t << " " << color::reset;
            std::cout << "\n";
            std::unordered_set<std::string> seen(templates.begin(), templates.end());
            for (const auto& t : detected)
                if (!seen.count(t)) { templates.push_back(t); seen.insert(t); }
        }
    }

    if (do_interactive) {
        if (!isatty(STDIN_FILENO)) {
            std::cerr << color::red << "Error: interactive mode requires a terminal\n" << color::reset;
            return 1;
        }
        store.all();
        std::vector<std::string> names;
        for (const auto& t : store.all()) names.push_back(t.name);

        std::unordered_set<std::string> presel(templates.begin(), templates.end());
        InteractiveSelector sel;
        auto chosen = sel.select(names, presel);
        if (chosen.empty()) return 0;
        templates = chosen;
    }

    if (templates.empty()) {
        print_header();
        std::cerr << color::red << "Error: no templates specified\n" << color::reset << "\n";
        print_usage();
        return 1;
    }

    generate(store, templates, output, append, do_preview, do_dedup, verbose);
    return 0;
}
