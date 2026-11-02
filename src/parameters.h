#ifndef SANDBOX_PARAMETERS_H
#define SANDBOX_PARAMETERS_H

#include "util.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct parameters {
    unsigned long mem_limit_in_kb;
    unsigned long time_limit_in_ms;
    std::string program_binary;
    std::string result_file;
    bool print_debug;
};

[[nodiscard]] inline std::optional<parameters> parse_parameters(const std::vector<char*>& args) {
    if (args.size() != 6)
        return std::nullopt;
    const auto mem_limit_in_kb = Util::parse_unsigned(args[1]);
    const auto time_limit_in_ms = Util::parse_unsigned(args[2]);
    if (not mem_limit_in_kb or not time_limit_in_ms)
        return std::nullopt;
    return parameters{.mem_limit_in_kb = *mem_limit_in_kb, .time_limit_in_ms = *time_limit_in_ms,
                      .program_binary = args[3], .result_file = args[4],
                      .print_debug = std::string_view{args[5]} == "true"};
}

#endif
