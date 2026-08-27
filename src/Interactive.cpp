#include "Interactive.hpp"
#include "Common.hpp"

#include <algorithm>
#include <csignal>
#include <iostream>
#include <poll.h>
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

    if (c == 1)                 return K_CTRL_A;
    if (c == 3 || c == 4)       return K_QUIT;
    if (c == 9)                 return K_TAB;
    if (c == 18)                return K_CTRL_R;
    if (c == 21)                return K_CTRL_U;
    if (c == 23)                return K_CTRL_W;
    if (c == '\r' || c == '\n') return K_ENTER;
    if (c == ' ')               return K_SPACE;
    if (c == 127 || c == 8)    return K_BACKSPACE;

    if (c == 27) {
        struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
        if (poll(&pfd, 1, 50) <= 0) {
            return K_QUIT;
        }

        char seq[4] = {0};
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return K_QUIT;

        if (seq[0] == '[') {
            if (poll(&pfd, 1, 50) <= 0) return K_QUIT;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) return K_QUIT;

            if (seq[1] == 'A') return K_UP;
            if (seq[1] == 'B') return K_DOWN;
            if (seq[1] == 'H') return K_HOME;
            if (seq[1] == 'F') return K_END;

            if (seq[1] >= '1' && seq[1] <= '8') {
                if (poll(&pfd, 1, 50) > 0 && read(STDIN_FILENO, &seq[2], 1) == 1 && seq[2] == '~') {
                    if (seq[1] == '1' || seq[1] == '7') return K_HOME;
                    if (seq[1] == '4' || seq[1] == '8') return K_END;
                    if (seq[1] == '3') return K_DELETE;
                    if (seq[1] == '5') return K_PAGE_UP;
                    if (seq[1] == '6') return K_PAGE_DOWN;
                }
            }
        } else if (seq[0] == 'O') {
            if (poll(&pfd, 1, 50) <= 0) return K_QUIT;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) return K_QUIT;
            if (seq[1] == 'A') return K_UP;
            if (seq[1] == 'B') return K_DOWN;
            if (seq[1] == 'H') return K_HOME;
            if (seq[1] == 'F') return K_END;
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
           color::gray, "  \u2191\u2193/PgUp/PgDn move  Space/Tab toggle  Ctrl+A all  Ctrl+U clear  Enter confirm  Esc quit", color::reset);

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
            case K_PAGE_UP:
                cursor = std::max(0, cursor - PAGE);
                break;
            case K_PAGE_DOWN:
                cursor = std::min((int)visible.size() - 1, cursor + PAGE);
                if (cursor < 0) cursor = 0;
                break;
            case K_HOME:
                cursor = 0;
                break;
            case K_END:
                cursor = std::max(0, (int)visible.size() - 1);
                break;
            case K_SPACE:
                if (!visible.empty()) {
                    const auto& name = visible[cursor];
                    if (selected.count(name)) selected.erase(name);
                    else selected.insert(name);
                }
                break;
            case K_TAB:
                if (!visible.empty()) {
                    const auto& name = visible[cursor];
                    if (selected.count(name)) selected.erase(name);
                    else selected.insert(name);
                    if (cursor < (int)visible.size() - 1) cursor++;
                }
                break;
            case K_CTRL_A: {
                if (!visible.empty()) {
                    bool all_sel = true;
                    for (const auto& name : visible) {
                        if (!selected.count(name)) { all_sel = false; break; }
                    }
                    for (const auto& name : visible) {
                        if (all_sel) selected.erase(name);
                        else selected.insert(name);
                    }
                }
                break;
            }
            case K_CTRL_R:
                selected.clear();
                break;
            case K_CTRL_U:
                if (!filter.empty()) {
                    filter.clear();
                    refilter();
                }
                break;
            case K_CTRL_W: {
                while (!filter.empty() && filter.back() == ' ') filter.pop_back();
                while (!filter.empty() && filter.back() != ' ') filter.pop_back();
                refilter();
                break;
            }
            case K_ENTER:
                done = true;
                break;
            case K_QUIT:
                done = cancelled = true;
                break;
            case K_BACKSPACE:
            case K_DELETE:
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
