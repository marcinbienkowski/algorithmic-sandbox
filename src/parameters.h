#ifndef SANDBOX_PARAMETERS_H
#define SANDBOX_PARAMETERS_H

#include <string>
#include <vector>

struct parameters {
    unsigned long mem_limit_in_kb;
    unsigned long time_limit_in_ms;
    std::string program_binary;
    std::string result_file;
    bool print_debug;
    explicit parameters(const std::vector<char*>& args) :
        mem_limit_in_kb(std::stoul(args[1])),
        time_limit_in_ms(std::stoul(args[2])),
        program_binary(args[3]),
        result_file(args[4]),
        print_debug(std::string{args[5]} == "true") {}
};

#endif
