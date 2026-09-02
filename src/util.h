#ifndef SANDBOX_UTIL_H
#define SANDBOX_UTIL_H

#include <format>
#include <source_location>
#include <stdexcept>


namespace Util {

    inline void ensure(bool condition, const std::string& error_string = "", const std::source_location& loc = std::source_location::current()) {
        if (!condition)
            throw std::runtime_error{std::format("at {} [{}]", loc.function_name(), error_string)};
    }

} // namespace Util

#endif
