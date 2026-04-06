#include "Interactive.hpp"
#include "Common.hpp"

#include <algorithm>
#include <iostream>
#include <termios.h>
#include <unistd.h>

InteractiveSelector::~InteractiveSelector() {
    disable_raw();
    delete orig_termios;
}

void InteractiveSelector::enable_raw() {
    orig_termios = new struct termios;
    tcgetattr(STDIN_FILENO, orig_termios);
    struct termios raw = *orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_active = true;
}

void InteractiveSelector::disable_raw() {
    if (raw_active && orig_termios) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
        raw_active = false;
    }
}

int InteractiveSelector::read_key() {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return K_QUIT;
    if (c == '\r' || c == '\n') return K_ENTER;
    if (c == ' ')               return K_SPACE;
    if (c == 'q' || c == 'Q')  return K_QUIT;
    if (c == 127 || c == 8)    return K_BACKSPACE;
    if (c == 27) {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return K_QUIT;
        if (seq[0] == '[') {
            if (read(STDIN_FILENO, &seq[1], 1) != 1) return K_QUIT;
            if (seq[1] == 'A') return K_UP;
            if (seq[1] == 'B') return K_DOWN;
        }
        return K_QUIT;
    }
    return (unsigned char)c;
}

void InteractiveSelector::move_up_and_clear(int lines) {
    for (int i = 0; i < lines; i++)
        std::cout << "\033[A\033[2K";
}

std::vector<std::string> InteractiveSelector::select(
    const std::vector<std::string>& all_names,
    const std::unordered_set<std::string>& preselected)
{
    if (all_names.empty()) {
        std::cout << color::yellow << "No templates available." << color::reset << "\n";
        return {};
    }

    std::unordered_set<std::string> selected = preselected;
    std::string filter;
    std::vector<std::string> visible = all_names;
    int cursor = 0;
    int scroll = 0;
    const int PAGE = 15;
    int rendered = 0;

    auto refilter = [&]() {
        visible.clear();
        if (filter.empty()) {
            visible = all_names;
        } else {
            std::string fl = filter;
            std::transform(fl.begin(), fl.end(), fl.begin(), ::tolower);
            for (const auto& name : all_names) {
                std::string n = name;
                std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                if (n.find(fl) != std::string::npos) visible.push_back(name);
            }
        }
        if (cursor >= (int)visible.size()) cursor = (int)visible.size() - 1;
        if (cursor < 0) cursor = 0;
        scroll = 0;
    };

    auto render = [&]() {
        if (rendered > 0) move_up_and_clear(rendered);
        rendered = 0;

        auto ln = [&](auto&&... args) {
            (std::cout << ... << args);
            std::cout << "\n";
            rendered++;
        };

        ln(color::bold, color::cyan, "Select templates", color::reset,
           color::gray, "  ↑↓ move  Space toggle  Enter confirm  q quit", color::reset);

        ln(color::gray, "Filter: ", color::reset,
           color::white, filter, color::reset,
           color::gray, filter.empty() ? " (type to filter)" : "", color::reset);

        if (visible.empty()) {
            ln(color::yellow, "  no matches", color::reset);
        } else {
            if (cursor < scroll) scroll = cursor;
            if (cursor >= scroll + PAGE) scroll = cursor - PAGE + 1;

            int end = std::min(scroll + PAGE, (int)visible.size());
            for (int i = scroll; i < end; i++) {
                const auto& name = visible[i];
                bool sel = selected.count(name) > 0;
                bool cur = (i == cursor);

                std::string prefix  = cur ? "> " : "  ";
                std::string box     = sel ? "[x] " : "[ ] ";
                std::string boxcol  = sel ? color::green : color::gray;
                std::string namecol;
                if      (cur && sel) namecol = color::bold + color::green;
                else if (cur)        namecol = color::bold + color::white;
                else if (sel)        namecol = color::green;
                else                 namecol = color::reset;

                ln(cur ? color::bold + color::white : color::gray, prefix, color::reset,
                   boxcol, box, color::reset,
                   namecol, name, color::reset);
            }

            if ((int)visible.size() > PAGE) {
                ln(color::gray, "  ... ", visible.size(), " total  (",
                   scroll + 1, "-", end, ")", color::reset);
            }
        }

        std::cout << color::gray << "Selected " << selected.size() << ": " << color::reset;
        int shown = 0;
        for (const auto& name : all_names) {
            if (!selected.count(name)) continue;
            if (shown >= 6) { std::cout << color::gray << "+" << (selected.size() - 6) << " more"; break; }
            std::cout << color::green << name << color::reset << " ";
            shown++;
        }
        std::cout << "\n";
        rendered++;

        std::cout.flush();
    };

    enable_raw();
    std::cout << "\033[?25l";
    render();

    bool done = false;
    bool cancelled = false;

    while (!done) {
        int key = read_key();
        switch (key) {
            case K_UP:
                if (cursor > 0) cursor--;
                break;
            case K_DOWN:
                if (cursor < (int)visible.size() - 1) cursor++;
                break;
            case K_SPACE:
                if (!visible.empty()) {
                    const auto& name = visible[cursor];
                    if (selected.count(name)) selected.erase(name);
                    else selected.insert(name);
                }
                break;
            case K_ENTER:
                done = true;
                break;
            case K_QUIT:
                done = cancelled = true;
                break;
            case K_BACKSPACE:
                if (!filter.empty()) { filter.pop_back(); refilter(); }
                break;
            default:
                if (key >= 32 && key < 127) { filter += (char)key; refilter(); }
                break;
        }
        if (!done) render();
    }

    std::cout << "\033[?25h";
    disable_raw();
    move_up_and_clear(rendered);

    if (cancelled) {
        std::cout << color::yellow << "Cancelled." << color::reset << "\n";
        return {};
    }

    std::vector<std::string> result;
    for (const auto& name : all_names) {
        if (selected.count(name)) result.push_back(name);
    }
    return result;
}
