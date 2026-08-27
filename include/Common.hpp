#pragma once

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <unistd.h>

namespace color {
    inline bool enabled = true;

    inline void init() {
        const char* no_color = std::getenv("NO_COLOR");
        if (no_color && no_color[0] != '\0') {
            enabled = false;
            return;
        }
        const char* term = std::getenv("TERM");
        if (term && std::string_view(term) == "dumb") {
            enabled = false;
            return;
        }
        if (!isatty(STDOUT_FILENO)) {
            enabled = false;
            return;
        }
        enabled = true;
    }

    inline void set_enabled(bool val) {
        enabled = val;
    }

    struct Code {
        std::string_view code;

        constexpr Code(std::string_view c) : code(c) {}

        friend std::ostream& operator<<(std::ostream& os, const Code& c) {
            if (enabled) os << c.code;
            return os;
        }

        operator std::string() const {
            return enabled ? std::string(code) : std::string();
        }

        operator std::string_view() const {
            return enabled ? code : std::string_view();
        }
    };

    inline std::string operator+(const Code& a, const Code& b) {
        if (!enabled) return "";
        return std::string(a.code) + std::string(b.code);
    }

    inline std::string operator+(const std::string& a, const Code& b) {
        if (!enabled) return a;
        return a + std::string(b.code);
    }

    inline std::string operator+(const Code& a, const std::string& b) {
        if (!enabled) return b;
        return std::string(a.code) + b;
    }

    inline constexpr Code reset   {"\033[0m"};
    inline constexpr Code bold    {"\033[1m"};
    inline constexpr Code red     {"\033[31m"};
    inline constexpr Code green   {"\033[32m"};
    inline constexpr Code yellow  {"\033[33m"};
    inline constexpr Code cyan    {"\033[36m"};
    inline constexpr Code white   {"\033[37m"};
    inline constexpr Code gray    {"\033[90m"};
}
