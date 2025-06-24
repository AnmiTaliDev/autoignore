#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>
#include <cstdlib>

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
    const std::string gray = "\033[90m";
}

class TemplateMixer {
private:
    std::vector<fs::path> template_paths;
    bool verbose = false;
    
    void init_template_paths() {
        const char* home = std::getenv("HOME");
        if (home) {
            template_paths.push_back(fs::path(home) / ".local/share/autoignore/template");
        }
        template_paths.push_back("/usr/local/share/autoignore/template");
        template_paths.push_back("/usr/share/autoignore/template");
    }
    
    struct TemplateInfo {
        std::string name;
        fs::path path;
        size_t size;
        std::string description;
        
        TemplateInfo(const std::string& n, const fs::path& p, size_t s) 
            : name(n), path(p), size(s) {}
    };
    
    std::vector<TemplateInfo> scan_templates() {
        std::unordered_map<std::string, TemplateInfo> templates;
        
        for (const auto& base_path : template_paths) {
            if (!fs::exists(base_path) || !fs::is_directory(base_path)) {
                continue;
            }
            
            for (const auto& entry : fs::directory_iterator(base_path)) {
                if (!entry.is_regular_file()) continue;
                
                std::string filename = entry.path().filename().string();
                if (!filename.ends_with(".gitignore")) continue;
                
                std::string name = filename.substr(0, filename.length() - 10);
                size_t size = entry.file_size();
                
                // Priority: user templates override system templates
                if (templates.find(name) == templates.end()) {
                    templates.emplace(name, TemplateInfo(name, entry.path(), size));
                }
            }
        }
        
        std::vector<TemplateInfo> result;
        result.reserve(templates.size());
        for (const auto& pair : templates) {
            result.push_back(pair.second);
        }
        
        std::sort(result.begin(), result.end(), 
                 [](const TemplateInfo& a, const TemplateInfo& b) {
                     return a.name < b.name;
                 });
        
        return result;
    }
    
    std::string read_template_content(const fs::path& template_path) {
        std::ifstream file(template_path);
        if (!file.is_open()) {
            return "";
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        return content;
    }
    
    std::string extract_description(const std::string& content) {
        std::istringstream stream(content);
        std::string line;
        std::vector<std::string> desc_lines;
        
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            if (line[0] != '#') break;
            
            // Remove leading # and whitespace
            std::string desc = line.substr(1);
            if (!desc.empty() && desc[0] == ' ') {
                desc = desc.substr(1);
            }
            
            if (!desc.empty() && desc != "gitignore" && 
                desc.find("Generated") == std::string::npos) {
                desc_lines.push_back(desc);
                if (desc_lines.size() >= 2) break; // First meaningful comment lines
            }
        }
        
        if (desc_lines.empty()) return "No description";
        
        std::string result = desc_lines[0];
        if (desc_lines.size() > 1) {
            result += " " + desc_lines[1];
        }
        
        // Truncate if too long
        if (result.length() > 60) {
            result = result.substr(0, 57) + "...";
        }
        
        return result;
    }
    
    void analyze_template_conflicts(const std::vector<std::string>& template_names) {
        std::unordered_map<std::string, std::vector<std::string>> pattern_sources;
        
        for (const auto& name : template_names) {
            auto templates = scan_templates();
            auto it = std::find_if(templates.begin(), templates.end(),
                                 [&name](const TemplateInfo& t) { return t.name == name; });
            
            if (it == templates.end()) continue;
            
            std::string content = read_template_content(it->path);
            std::istringstream stream(content);
            std::string line;
            
            while (std::getline(stream, line)) {
                // Skip comments and empty lines
                if (line.empty() || line[0] == '#') continue;
                
                // Trim whitespace
                line.erase(0, line.find_first_not_of(" \t"));
                line.erase(line.find_last_not_of(" \t") + 1);
                
                if (!line.empty()) {
                    pattern_sources[line].push_back(name);
                }
            }
        }
        
        // Find conflicts
        std::vector<std::pair<std::string, std::vector<std::string>>> conflicts;
        for (const auto& pair : pattern_sources) {
            if (pair.second.size() > 1) {
                conflicts.push_back(pair);
            }
        }
        
        if (!conflicts.empty()) {
            std::cout << color::yellow << "Potential pattern conflicts detected:" << color::reset << std::endl;
            for (const auto& conflict : conflicts) {
                std::cout << "  " << color::cyan << conflict.first << color::reset 
                          << color::gray << " (from: ";
                for (size_t i = 0; i < conflict.second.size(); ++i) {
                    std::cout << conflict.second[i];
                    if (i < conflict.second.size() - 1) std::cout << ", ";
                }
                std::cout << ")" << color::reset << std::endl;
            }
            std::cout << std::endl;
        }
    }
    
    void optimize_template_order(std::vector<std::string>& template_names) {
        // Simple heuristic: put broader templates first, specific ones last
        std::sort(template_names.begin(), template_names.end(), 
                 [](const std::string& a, const std::string& b) {
                     // Common base templates should come first
                     static const std::vector<std::string> base_templates = {
                         "global", "macos", "windows", "linux"
                     };
                     
                     bool a_is_base = std::find(base_templates.begin(), base_templates.end(), a) 
                                     != base_templates.end();
                     bool b_is_base = std::find(base_templates.begin(), base_templates.end(), b) 
                                     != base_templates.end();
                     
                     if (a_is_base != b_is_base) {
                         return a_is_base; // Base templates first
                     }
                     
                     return a < b; // Alphabetical otherwise
                 });
    }
    
public:
    TemplateMixer() {
        init_template_paths();
    }
    
    void set_verbose(bool v) { verbose = v; }
    
    void list_templates_detailed() {
        auto templates = scan_templates();
        
        if (templates.empty()) {
            std::cout << color::yellow << "No templates found." << color::reset << std::endl;
            return;
        }
        
        std::cout << color::bold << "Available templates (" << templates.size() << "):" << color::reset << std::endl;
        std::cout << std::endl;
        
        for (const auto& tmpl : templates) {
            std::string content = read_template_content(tmpl.path);
            std::string description = extract_description(content);
            
            std::cout << color::green << color::bold << tmpl.name << color::reset;
            
            // Size info
            std::string size_str;
            if (tmpl.size < 1024) {
                size_str = std::to_string(tmpl.size) + "B";
            } else if (tmpl.size < 1024 * 1024) {
                size_str = std::to_string(tmpl.size / 1024) + "KB";
            } else {
                size_str = std::to_string(tmpl.size / (1024 * 1024)) + "MB";
            }
            
            std::cout << color::gray << " (" << size_str << ")" << color::reset;
            
            // Source location indicator
            bool is_user_template = tmpl.path.string().find("/.local/") != std::string::npos;
            if (is_user_template) {
                std::cout << color::blue << " [user]" << color::reset;
            } else {
                std::cout << color::cyan << " [system]" << color::reset;
            }
            
            std::cout << std::endl;
            std::cout << "  " << color::gray << description << color::reset << std::endl;
            std::cout << std::endl;
        }
    }
    
    void preview_mix(const std::vector<std::string>& template_names) {
        if (template_names.empty()) {
            std::cerr << color::red << "No templates specified for preview" << color::reset << std::endl;
            return;
        }
        
        std::cout << color::bold << "Preview of template mix:" << color::reset << std::endl;
        for (const auto& name : template_names) {
            std::cout << "  " << color::green << name << color::reset;
        }
        std::cout << std::endl << std::endl;
        
        // Analyze conflicts
        if (template_names.size() > 1) {
            analyze_template_conflicts(template_names);
        }
        
        // Show combined stats
        size_t total_lines = 0;
        size_t total_patterns = 0;
        
        auto templates = scan_templates();
        for (const auto& name : template_names) {
            auto it = std::find_if(templates.begin(), templates.end(),
                                 [&name](const TemplateInfo& t) { return t.name == name; });
            
            if (it == templates.end()) {
                std::cout << color::yellow << "Warning: Template '" << name 
                          << "' not found" << color::reset << std::endl;
                continue;
            }
            
            std::string content = read_template_content(it->path);
            std::istringstream stream(content);
            std::string line;
            size_t lines = 0, patterns = 0;
            
            while (std::getline(stream, line)) {
                lines++;
                if (!line.empty() && line[0] != '#') {
                    line.erase(0, line.find_first_not_of(" \t"));
                    if (!line.empty()) patterns++;
                }
            }
            
            total_lines += lines;
            total_patterns += patterns;
            
            if (verbose) {
                std::cout << color::cyan << name << color::reset << ": " 
                          << lines << " lines, " << patterns << " patterns" << std::endl;
            }
        }
        
        std::cout << color::bold << "Total: " << color::reset 
                  << total_lines << " lines, " << total_patterns << " ignore patterns" << std::endl;
    }
    
    std::vector<std::string> suggest_templates(const std::string& project_hint = "") {
        std::vector<std::string> suggestions;
        auto templates = scan_templates();
        
        if (project_hint.empty()) {
            // Default suggestions
            std::vector<std::string> common = {"global", "macos", "windows", "linux"};
            for (const auto& name : common) {
                auto it = std::find_if(templates.begin(), templates.end(),
                                     [&name](const TemplateInfo& t) { return t.name == name; });
                if (it != templates.end()) {
                    suggestions.push_back(name);
                }
            }
        } else {
            // Project-specific suggestions based on hint
            std::string hint_lower = project_hint;
            std::transform(hint_lower.begin(), hint_lower.end(), hint_lower.begin(), ::tolower);
            
            for (const auto& tmpl : templates) {
                std::string name_lower = tmpl.name;
                std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
                
                if (hint_lower.find(name_lower) != std::string::npos ||
                    name_lower.find(hint_lower) != std::string::npos) {
                    suggestions.push_back(tmpl.name);
                }
            }
        }
        
        return suggestions;
    }
    
    void show_template_statistics() {
        auto templates = scan_templates();
        
        if (templates.empty()) {
            std::cout << color::yellow << "No templates available for statistics" << color::reset << std::endl;
            return;
        }
        
        size_t user_templates = 0;
        size_t system_templates = 0;
        size_t total_size = 0;
        
        for (const auto& tmpl : templates) {
            total_size += tmpl.size;
            
            if (tmpl.path.string().find("/.local/") != std::string::npos) {
                user_templates++;
            } else {
                system_templates++;
            }
        }
        
        std::cout << color::bold << "Template Statistics:" << color::reset << std::endl;
        std::cout << "  Total templates: " << color::green << templates.size() << color::reset << std::endl;
        std::cout << "  User templates: " << color::blue << user_templates << color::reset << std::endl;
        std::cout << "  System templates: " << color::cyan << system_templates << color::reset << std::endl;
        std::cout << "  Total size: " << color::yellow;
        
        if (total_size < 1024) {
            std::cout << total_size << "B";
        } else if (total_size < 1024 * 1024) {
            std::cout << (total_size / 1024) << "KB";
        } else {
            std::cout << (total_size / (1024 * 1024)) << "MB";
        }
        std::cout << color::reset << std::endl;
        
        std::cout << std::endl;
        std::cout << color::bold << "Template locations:" << color::reset << std::endl;
        for (const auto& path : template_paths) {
            std::cout << "  " << color::cyan << path << color::reset;
            if (fs::exists(path) && fs::is_directory(path)) {
                auto count = std::distance(fs::directory_iterator(path), fs::directory_iterator{});
                std::cout << color::gray << " (" << count << " files)" << color::reset;
            } else {
                std::cout << color::gray << " (not found)" << color::reset;
            }
            std::cout << std::endl;
        }
    }
    
    std::vector<std::string> optimize_template_selection(std::vector<std::string> template_names) {
        if (verbose) {
            std::cout << color::blue << "Optimizing template order..." << color::reset << std::endl;
        }
        
        optimize_template_order(template_names);
        
        if (verbose && template_names.size() > 1) {
            std::cout << "Optimized order: ";
            for (size_t i = 0; i < template_names.size(); ++i) {
                std::cout << color::green << template_names[i] << color::reset;
                if (i < template_names.size() - 1) std::cout << " → ";
            }
            std::cout << std::endl;
        }
        
        return template_names;
    }
};