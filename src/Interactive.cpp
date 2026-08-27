#include "Interactive.hpp"
#include "Common.hpp"

#include <algorithm>
#include <csignal>
#include <iostream>
#include <termios.h>
#include <unistd.h>

namespace {
    struct TerminalGuard {
        struct termios orig_termios {};
        bool raw_active = false;
        static inline TerminalGuard* active_instance = nullptr;
        struct sigaction old_sigint {};
        struct sigaction old_sigterm {};

        static void signal_handler(int sig) {
            if (active_instance) {
                active_instance->restore();
            }
            std::signal(sig, SIG_DFL);
            std::raise(sig);
        }

        TerminalGuard() {
            if (tcgetattr(STDIN_FILENO, &orig_termios) == 0) {
                struct termios raw = orig_termios;
                raw.c_lflag &= ~(ECHO | ICANON);
                raw.c_cc[VMIN] = 1;
                raw.c_cc[VTIME] = 0;
                if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0) {
                    raw_active = true;
                }
            }
            std::cout << "\033[?25l";
            std::cout.flush();

            active_instance = this;

            struct sigaction sa {};
            sa.sa_handler = signal_handler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            sigaction(SIGINT, &sa, &old_sigint);
            sigaction(SIGTERM, &sa, &old_sigterm);
        }

        void restore() {
            if (raw_active) {
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
                raw_active = false;
            }
            std::cout << "\033[?25h";
            std::cout.flush();
            sigaction(SIGINT, &old_sigint, nullptr);
            sigaction(SIGTERM, &old_sigterm, nullptr);
            if (active_instance == this) active_instance = nullptr;
        }

        ~TerminalGuard() {
            restore();
        }

        TerminalGuard(const TerminalGuard&) = delete;
        TerminalGuard& operator=(const TerminalGuard&) = delete;
    };
}

int InteractiveSelector::read_key() {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return K_QUIT;
    if (c == 3 || c == 4)       return K_QUIT;
    if (c == '\r' || c == '\n') return K_ENTER;
    if (c == ' ')               return K_SPACE;
    if (c == 127 || c == 8)    return K_BACKSPACE;
    if (c == 27) {
        struct termios orig_t, nonblock_t;
        tcgetattr(STDIN_FILENO, &orig_t);
        nonblock_t = orig_t;
        nonblock_t.c_cc[VMIN] = 0;
        nonblock_t.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSANOW, &nonblock_t);

        char seq[2];
        ssize_t n1 = read(STDIN_FILENO, &seq[0], 1);
        ssize_t n2 = (n1 == 1) ? read(STDIN_FILENO, &seq[1], 1) : 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_t);

        if (n1 == 1 && seq[0] == '[') {
            if (n2 == 1) {
                if (seq[1] == 'A') return K_UP;
                if (seq[1] == 'B') return K_DOWN;
            }
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

    struct Item {
        std::string name;
        std::string lower;
    };

    std::vector<Item> items;
    items.reserve(all_names.size());
    for (const auto& name : all_names) {
        std::string l = name;
        std::transform(l.begin(), l.end(), l.begin(), ::tolower);
        items.push_back({name, std::move(l)});
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
            for (const auto& item : items) {
                if (item.lower.find(fl) != std::string::npos) {
                    visible.push_back(item.name);
                }
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
           color::gray, "  \u2191\u2193 move  Space toggle  Enter confirm  Esc/Ctrl+C quit", color::reset);

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

    TerminalGuard guard;
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

    guard.restore();
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
