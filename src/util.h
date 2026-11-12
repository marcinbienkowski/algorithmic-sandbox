#ifndef SANDBOX_UTIL_H
#define SANDBOX_UTIL_H

#include <charconv>
#include <format>
#include <optional>
#include <source_location>
#include <stdexcept>
#include <string_view>
#include <system_error>


namespace Util {

    inline void ensure(bool condition, std::string_view error_string = "", const std::source_location& loc = std::source_location::current()) {
        if (not condition)
            throw std::runtime_error{std::format("at {} [{}]", loc.function_name(), error_string)};
    }

    [[nodiscard]] inline std::optional<unsigned long> parse_unsigned(std::string_view s) {
        unsigned long value = 0;
        const auto [end, ec] = std::from_chars(s.begin(), s.end(), value);
        if (ec != std::errc{} or end != s.end())
            return std::nullopt;
        return value;
    }

} // namespace Util

#endif
