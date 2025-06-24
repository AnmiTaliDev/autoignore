#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <cstdlib>
#include <getopt.h>

namespace fs = std::filesystem;
namespace color {
    const std::string reset = "\033[0m";
    const std::string bold = "\033[1m";
    const std::string red = "\033[31m";
    const std::string green = "\033[32m";
    const std::string yellow = "\033[33m";
    const std::string blue = "\033[34m";
    const std::string magenta = "\033[35m";
    const std::string cyan = "\033[36m";
    const std::string white = "\033[37m";
    const std::string gray = "\033[90m";
}

class AutoIgnore {
private:
    std::vector<fs::path> template_paths;
    bool verbose = false;
    
    void init_template_paths() {
        // User-specific templates
        const char* home = std::getenv("HOME");
        if (home) {
            template_paths.push_back(fs::path(home) / ".local/share/autoignore/template");
        }
        
        // System-wide templates
        template_paths.push_back("/usr/local/share/autoignore/template");
        template_paths.push_back("/usr/share/autoignore/template");
    }
    
    std::vector<std::string> get_available_templates() {
        std::unordered_set<std::string> templates;
        
        for (const auto& path : template_paths) {
            if (!fs::exists(path) || !fs::is_directory(path)) {
                continue;
            }
            
            for (const auto& entry : fs::directory_iterator(path)) {
                if (entry.is_regular_file()) {
                    std::string filename = entry.path().filename().string();
                    if (filename.ends_with(".gitignore")) {
                        templates.insert(filename.substr(0, filename.length() - 10));
                    }
                }
            }
        }
        
        return std::vector<std::string>(templates.begin(), templates.end());
    }
    
    std::string read_template(const std::string& template_name) {
        for (const auto& path : template_paths) {
            fs::path template_file = path / (template_name + ".gitignore");
            if (fs::exists(template_file)) {
                std::ifstream file(template_file);
                if (file.is_open()) {
                    std::string content((std::istreambuf_iterator<char>(file)),
                                      std::istreambuf_iterator<char>());
                    return content;
                }
            }
        }
        return "";
    }
    
    void print_header() {
        std::cout << color::bold << color::cyan << "autoignore" << color::reset 
                  << color::gray << " - GitIgnore template generator" << color::reset << std::endl;
        std::cout << color::gray << "Author: AnmiTaliDev | License: Apache 2.0" << color::reset << std::endl;
        std::cout << std::endl;
    }
    
    void print_usage() {
        std::cout << color::bold << "Usage:" << color::reset << std::endl;
        std::cout << "  autoignore [OPTIONS] [TEMPLATES...]" << std::endl;
        std::cout << std::endl;
        std::cout << color::bold << "Options:" << color::reset << std::endl;
        std::cout << "  -l, --list      List available templates" << std::endl;
        std::cout << "  -o, --output    Output file (default: .gitignore)" << std::endl;
        std::cout << "  -a, --append    Append to existing file instead of overwriting" << std::endl;
        std::cout << "  -v, --verbose   Verbose output" << std::endl;
        std::cout << "  -h, --help      Show this help message" << std::endl;
        std::cout << std::endl;
        std::cout << color::bold << "Examples:" << color::reset << std::endl;
        std::cout << "  autoignore cpp python" << std::endl;
        std::cout << "  autoignore -l" << std::endl;
        std::cout << "  autoignore -o .gitignore_custom node" << std::endl;
        std::cout << "  autoignore -a rust" << std::endl;
    }
    
    void list_templates() {
        auto templates = get_available_templates();
        
        if (templates.empty()) {
            std::cout << color::yellow << "No templates found." << color::reset << std::endl;
            std::cout << color::gray << "Templates should be located in:" << color::reset << std::endl;
            for (const auto& path : template_paths) {
                std::cout << "  " << color::cyan << path << color::reset << std::endl;
            }
            return;
        }
        
        std::sort(templates.begin(), templates.end());
        
        std::cout << color::bold << "Available templates:" << color::reset << std::endl;
        for (const auto& tmpl : templates) {
            std::cout << "  " << color::green << tmpl << color::reset << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << color::gray << "Template locations:" << color::reset << std::endl;
        for (const auto& path : template_paths) {
            if (fs::exists(path)) {
                std::cout << "  " << color::cyan << path << color::reset;
                if (fs::is_directory(path)) {
                    auto count = std::distance(fs::directory_iterator(path), fs::directory_iterator{});
                    std::cout << color::gray << " (" << count << " files)" << color::reset;
                }
                std::cout << std::endl;
            } else {
                std::cout << "  " << color::gray << path << " (not found)" << color::reset << std::endl;
            }
        }
    }
    
    void generate_gitignore(const std::vector<std::string>& templates, 
                          const std::string& output_file, bool append) {
        std::ofstream file;
        
        if (append) {
            file.open(output_file, std::ios::app);
            if (verbose) {
                std::cout << color::blue << "Appending to " << output_file << color::reset << std::endl;
            }
        } else {
            file.open(output_file, std::ios::trunc);
            if (verbose) {
                std::cout << color::blue << "Writing to " << output_file << color::reset << std::endl;
            }
        }
        
        if (!file.is_open()) {
            std::cerr << color::red << "Error: Cannot open file " << output_file << color::reset << std::endl;
            return;
        }
        
        // Header
        file << "# Generated by autoignore" << std::endl;
        file << "# Templates: ";
        for (size_t i = 0; i < templates.size(); ++i) {
            file << templates[i];
            if (i < templates.size() - 1) file << ", ";
        }
        file << std::endl;
        file << std::endl;
        
        bool any_template_found = false;
        
        for (const auto& template_name : templates) {
            std::string content = read_template(template_name);
            
            if (content.empty()) {
                std::cerr << color::yellow << "Warning: Template '" << template_name 
                          << "' not found" << color::reset << std::endl;
                continue;
            }
            
            any_template_found = true;
            
            if (verbose) {
                std::cout << color::green << "Adding template: " << template_name << color::reset << std::endl;
            }
            
            file << "# " << template_name << std::endl;
            file << content;
            if (!content.ends_with('\n')) {
                file << std::endl;
            }
            file << std::endl;
        }
        
        file.close();
        
        if (any_template_found) {
            std::cout << color::green << "Successfully generated " << output_file << color::reset << std::endl;
        } else {
            std::cerr << color::red << "Error: No valid templates found" << color::reset << std::endl;
        }
    }

public:
    AutoIgnore() {
        init_template_paths();
    }
    
    int run(int argc, char* argv[]) {
        bool list_mode = false;
        bool append_mode = false;
        std::string output_file = ".gitignore";
        
        static struct option long_options[] = {
            {"list", no_argument, 0, 'l'},
            {"output", required_argument, 0, 'o'},
            {"append", no_argument, 0, 'a'},
            {"verbose", no_argument, 0, 'v'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };
        
        int option_index = 0;
        int c;
        
        while ((c = getopt_long(argc, argv, "lo:avh", long_options, &option_index)) != -1) {
            switch (c) {
                case 'l':
                    list_mode = true;
                    break;
                case 'o':
                    output_file = optarg;
                    break;
                case 'a':
                    append_mode = true;
                    break;
                case 'v':
                    verbose = true;
                    break;
                case 'h':
                    print_header();
                    print_usage();
                    return 0;
                case '?':
                    std::cerr << color::red << "Invalid option" << color::reset << std::endl;
                    return 1;
                default:
                    break;
            }
        }
        
        if (list_mode) {
            print_header();
            list_templates();
            return 0;
        }
        
        std::vector<std::string> templates;
        for (int i = optind; i < argc; ++i) {
            templates.push_back(argv[i]);
        }
        
        if (templates.empty()) {
            print_header();
            std::cerr << color::red << "Error: No templates specified" << color::reset << std::endl;
            std::cout << std::endl;
            print_usage();
            return 1;
        }
        
        if (verbose) {
            print_header();
        }
        
        generate_gitignore(templates, output_file, append_mode);
        
        return 0;
    }
};

int main(int argc, char* argv[]) {
    AutoIgnore app;
    return app.run(argc, argv);
}