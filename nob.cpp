#include <format>
#include <iostream>

#define NOB_IMPLEMENTATION
#include "nob.hpp"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    const char *main_cpp = "main.cpp";
    const char *main_o = "debug_main";

    const char *program = nob_shift_args(&argc, &argv);
    const char *benchmarking = argc > 0 ? nob_shift_args(&argc, &argv) : nullptr;
    if (benchmarking) {
        main_o = "main";
    }
    Nob_Cmd cmd = {
        CXX_COMPILER, NOB_CPPSTD_STR, "-o", main_o, main_cpp
        //, "-L./", "-lraylib", "-I./include"
    };

    if (!benchmarking)
        nob_cmd_append(cmd, "-fsanitize=undefined,address", "-ggdb", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-Wno-unused-parameter");
    else
        nob_cmd_append(cmd, "-O3", "-march=native");

    if (nob_needs_rebuild1(main_o, main_cpp)) {
        for (auto &i : cmd) {
            std::cout << i << " ";
        }
        std::cout << std::endl;
        if (!nob_cmd_run_sync(cmd))
            return 1;
    }

    cmd.clear();

    cmd.push_back(std::format("./{}", main_o));

    nob_cmd_run_sync(cmd);

    return 0;
}