#include <filesystem>
#include <format>
#include <iostream>
#include <string>

/* to compile run
g++ -std=c++23 nob.cpp -o nob
*/

/*
to compile for linux, provide no arguments, to compile for windows provide "win" as an argument
*/

#define NOB_IMPLEMENTATION
#include "nob.hpp"

static const char *raylib_modules[] = {
    "rcore",
    "raudio",
    "rglfw",
    "rmodels",
    "rshapes",
    "rtext",
    "rtextures",
    "utils",
};

std::vector<std::string> get_c_files(std::string dir) {
    std::vector<std::string> c_files;
    for (int i = 0; i < sizeof(raylib_modules) / sizeof(raylib_modules[0]); i++) {
        std::string module = raylib_modules[i];
        std::string module_dir = std::format("{}/{}.c", dir, module);

        c_files.push_back(module_dir);
    }
    return c_files;
}

bool find_arg(std::span<char *> args, const char *arg) {
    for (auto &a : args) {
        if (std::string(a) == std::string(arg))
            return true;
    }
    return false;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    const char *program = nob_shift_args(&argc, &argv);
    std::span<char *> args = std::span<char *>(argv, argc);
    bool win = find_arg(args, "win");
    bool rebuild = find_arg(args, "rebuild");

    std::string src_dir = "./src";
    std::vector<std::string> c_files = get_c_files(src_dir);
    std::string librayliba = "./build_lin/libraylib.a";

    if (win)
        librayliba = "./build_win/libraylib.lib";

    Nob_Cmd cmd = {
        "gcc",
        "-ggdb", "-DPLATFORM_DESKTOP", "-fPIC",
        "-I./src/external/glfw/include",
        "-O3",
        "-c"};

    if (win) {
        cmd[0] = "x86_64-w64-mingw32-gcc";
    }

    // i have no clue why i have to iterate through every c file in order to compile them all
    for (int i = 0; i < sizeof(raylib_modules) / sizeof(raylib_modules[0]); i++) {
        Nob_Cmd new_cmd = cmd;
        new_cmd.push_back(std::format("./src/{}.c", raylib_modules[i]));
        new_cmd.push_back("-o");
        std::string o_file;
        if (win)
            o_file = std::format("./build_win/{}.o", raylib_modules[i]);
        else
            o_file = std::format("./build_lin/{}.o", raylib_modules[i]);
        new_cmd.push_back(o_file);

        if (nob_needs_rebuild1(o_file.c_str(), c_files[i].c_str()) || rebuild) {
            Nob_Cmd dir = {"mkdir"};

            if (win)
                dir.push_back("build_win");
            else
                dir.push_back("build_lin");

            nob_cmd_run_sync(dir);

            if (!nob_cmd_run_sync(new_cmd))
                return 1;
        }
    }

    if (nob_needs_rebuild(librayliba.c_str(), c_files)) {
        Nob_Cmd new_cmd = {
            "gcc-ar",
            "rcs",
            "-flto",
            "-o"};

        if (win)
            new_cmd[0] = "x86_64-w64-mingw32-gcc-ar";

        new_cmd.push_back(librayliba);
        for (int i = 0; i < sizeof(raylib_modules) / sizeof(raylib_modules[0]); i++) {
            std::string o_file;
            if (win)
                o_file = std::format("./build_win/{}.o", raylib_modules[i]);
            else
                o_file = std::format("./build_lin/{}.o", raylib_modules[i]);

            new_cmd.push_back(o_file);
        }
        if (!nob_cmd_run_sync(new_cmd))
            return 1;
    }
    if (win)
        std::system("cp ./build_win/libraylib.lib ../libraylib.lib");
    else
        std::system("cp ./build_lin/libraylib.a ../libraylib.a");
    std::cout << "Done!" << std::endl;
    return 0;
}