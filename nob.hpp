// This is a complete backward incompatible rewrite of https://github.com/tsoding/nobuild
// because I'm really unhappy with the direction it is going. It's gonna sit in this repo
// until it's matured enough and then I'll probably extract it to its own repo.

// Copyright 2023 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef NOB_H_
#define NOB_H_

#define NOB_ASSERT assert
#define NOB_FREE free

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define _WINUSER_
#define _WINGDI_
#define _IMM_
#define _WINCON_
#include <direct.h>
#include <shellapi.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#define NOB_LINE_END "\r\n"
#else
#define NOB_LINE_END "\n"
#endif

#define NOB_ARRAY_LEN(array) (sizeof(array) / sizeof(array[0]))
#define NOB_ARRAY_GET(array, index) \
    (NOB_ASSERT(index >= 0), NOB_ASSERT(index < NOB_ARRAY_LEN(array)), array[index])

typedef enum {
    NOB_INFO,
    NOB_WARNING,
    NOB_ERROR,
} Nob_Log_Level;

void nob_log(Nob_Log_Level level, const char *fmt, ...);

// It is an equivalent of shift command from bash. It basically pops a command line
// argument from the beginning.
char *nob_shift_args(int *argc, char ***argv);

typedef enum {
    NOB_FILE_REGULAR = 0,
    NOB_FILE_DIRECTORY,
    NOB_FILE_SYMLINK,
    NOB_FILE_OTHER,
} Nob_File_Type;

bool nob_mkdir_if_not_exists(const char *path);
bool nob_copy_file(const char *src_path, const char *dst_path);
bool nob_copy_directory_recursively(const char *src_path, const char *dst_path);
bool nob_read_entire_dir(const char *parent, std::vector<std::string> children);
bool nob_write_entire_file(const char *path, void *data, size_t size);
Nob_File_Type nob_get_file_type(const char *path);

#define nob_return_defer(value) \
    do {                        \
        result = (value);       \
        goto defer;             \
    } while (0)

// Initial capacity of a dynamic array
#define NOB_DA_INIT_CAP 256

bool nob_read_entire_file(const char *path, std::string sb);

// Process handle
#ifdef _WIN32
typedef HANDLE Nob_Proc;
#define NOB_INVALID_PROC INVALID_HANDLE_VALUE
#else
typedef int Nob_Proc;
#define NOB_INVALID_PROC (-1)
#endif  // _WIN32

typedef std::vector<Nob_Proc> Nob_Procs;

bool nob_procs_wait(std::vector<Nob_Proc> procs);

// Wait until the process has finished
bool nob_proc_wait(Nob_Proc proc);

// A command - the main workhorse of Nob. Nob is all about building commands an running them
typedef std::vector<std::string> Nob_Cmd;

// Render a string representation of a command into a string builder. Keep in mind the the
// string builder is not NULL-terminated by default. Use nob_sb_append_null if you plan to
// use it as a C string.
void nob_cmd_render(Nob_Cmd cmd, std::string &render);

// Run command asynchronously
Nob_Proc nob_cmd_run_async(Nob_Cmd &cmd);

// Run command synchronously
bool nob_cmd_run_sync(Nob_Cmd &cmd);

void nob_cmd_append_many(Nob_Cmd &cmd, ...);
#define nob_cmd_append(cmd, ...) nob_cmd_append_many(cmd, __VA_ARGS__, (const char *)NULL)

#ifndef NOB_TEMP_CAPACITY
#define NOB_TEMP_CAPACITY (8 * 1024 * 1024)
#endif  // NOB_TEMP_CAPACITY
char *nob_temp_strdup(const char *cstr);
void *nob_temp_alloc(size_t size);
char *nob_temp_sprintf(const char *format, ...);
void nob_temp_reset(void);
size_t nob_temp_save(void);
void nob_temp_rewind(size_t checkpoint);

int is_path1_modified_after_path2(const char *path1, const char *path2);
bool nob_rename(const char *old_path, const char *new_path);
int nob_needs_rebuild(std::string output_path, std::vector<std::string> input_paths);
int nob_needs_rebuild1(const char *output_path, const char *input_path);
int nob_file_exists(const char *file_path);

#ifndef NOB_CPPSTD
#define NOB_CPPSTD 23
#endif

#define STR2(x) #x
#define STR(x) STR2(x)
#define NOB_CPPSTD_STR "-std=c++" STR(NOB_CPPSTD)

// TODO: add MinGW support for Go Rebuild Urself™ Technology
#ifndef NOB_REBUILD_URSELF
#if _WIN32
#if defined(__GNUC__)
#define CXX_COMPILER "g++"
#define NOB_REBUILD_URSELF(binary_path, source_path) CXX_COMPILER, NOB_CPPSTD_STR, "-o", binary_path, source_path
#elif defined(__clang__)
#define CXX_COMPILER "clang++"
#define NOB_REBUILD_URSELF(binary_path, source_path) CXX_COMPILER, NOB_CPPSTD_STR, "-o", binary_path, source_path
#elif defined(_MSC_VER)
#define CXX_COMPILER "cl.exe"
#define NOB_REBUILD_URSELF(binary_path, source_path) CXX_COMPILER, source_path
#endif
#else
#define CXX_COMPILER "g++"
#define NOB_REBUILD_URSELF(binary_path, source_path) CXX_COMPILER, NOB_CPPSTD_STR, "-o", binary_path, source_path
#endif
#endif

// Go Rebuild Urself™ Technology
//
//   How to use it:
//     int main(int argc, char** argv) {
//         GO_REBUILD_URSELF(argc, argv);
//         // actual work
//         return 0;
//     }
//
//   After your added this macro every time you run ./nobuild it will detect
//   that you modified its original source code and will try to rebuild itself
//   before doing any actual work. So you only need to bootstrap your build system
//   once.
//
//   The modification is detected by comparing the last modified times of the executable
//   and its source code. The same way the make utility usually does it.
//
//   The rebuilding is done by using the REBUILD_URSELF macro which you can redefine
//   if you need a special way of bootstraping your build system. (which I personally
//   do not recommend since the whole idea of nobuild is to keep the process of bootstrapping
//   as simple as possible and doing all of the actual work inside of the nobuild)
//
#define NOB_GO_REBUILD_URSELF(argc, argv)                                        \
    do {                                                                         \
        std::vector<std::string> source_path = {__FILE__};                       \
        assert(argc >= 1);                                                       \
        const char *binary_path = argv[0];                                       \
                                                                                 \
        int rebuild_is_needed = nob_needs_rebuild(binary_path, source_path);     \
        if (rebuild_is_needed < 0)                                               \
            exit(1);                                                             \
        if (rebuild_is_needed) {                                                 \
            std::string sb;                                                      \
            sb.append(binary_path);                                              \
            sb.append(".old");                                                   \
            if (!nob_rename(binary_path, sb.c_str()))                            \
                exit(1);                                                         \
                                                                                 \
            Nob_Cmd rebuild = {NOB_REBUILD_URSELF(binary_path, source_path[0])}; \
            bool rebuild_succeeded = nob_cmd_run_sync(rebuild);                  \
            rebuild.clear();                                                     \
            if (!rebuild_succeeded) {                                            \
                nob_rename(sb.c_str(), binary_path);                             \
                exit(1);                                                         \
            }                                                                    \
                                                                                 \
            Nob_Cmd cmd;                                                         \
            for (int i = 0; i < argc; ++i)                                       \
                cmd.push_back(std::string(argv[i]));                             \
            if (!nob_cmd_run_sync(cmd))                                          \
                exit(1);                                                         \
            exit(0);                                                             \
        }                                                                        \
    } while (0)  // The implementation idea is stolen from https://github.com/zhiayang/nabs

typedef struct
{
    size_t count;
    const char *data;
} Nob_String_View;

const char *nob_temp_sv_to_cstr(Nob_String_View sv);

Nob_String_View nob_sv_chop_by_delim(Nob_String_View *sv, char delim);
Nob_String_View nob_sv_trim(Nob_String_View sv);
bool nob_sv_eq(Nob_String_View a, Nob_String_View b);
Nob_String_View nob_sv_from_cstr(const char *cstr);
Nob_String_View nob_sv_from_parts(const char *data, size_t count);

// printf macros for String_View
#ifndef SV_Fmt
#define SV_Fmt "%.*s"
#endif  // SV_Fmt
#ifndef SV_Arg
#define SV_Arg(sv) (int)(sv).count, (sv).data
#endif  // SV_Arg
// USAGE:
//   String_View name = ...;
//   printf("Name: "SV_Fmt"\n", SV_Arg(name));

// minirent.h HEADER BEGIN ////////////////////////////////////////
// Copyright 2021 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// ============================================================
//
// minirent — 0.0.1 — A subset of dirent interface for Windows.
//
// https://github.com/tsoding/minirent
//
// ============================================================
//
// ChangeLog (https://semver.org/ is implied)
//
//    0.0.2 Automatically include dirent.h on non-Windows
//          platforms
//    0.0.1 First Official Release

#ifndef _WIN32
#include <dirent.h>
#else  // _WIN32

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

struct dirent {
    char d_name[MAX_PATH + 1];
};

typedef struct DIR DIR;

DIR *opendir(const char *dirpath);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
#endif  // _WIN32
// minirent.h HEADER END ////////////////////////////////////////

#endif  // NOB_H_

#define NOB_IMPLEMENTATION
#ifdef NOB_IMPLEMENTATION

static size_t nob_temp_size = 0;
static char nob_temp[NOB_TEMP_CAPACITY] = {0};

bool nob_mkdir_if_not_exists(const char *path) {
#ifdef _WIN32
    int result = mkdir(path);
#else
    int result = mkdir(path, 0755);
#endif
    if (result < 0) {
        if (errno == EEXIST) {
            nob_log(NOB_INFO, "directory `%s` already exists", path);
            return true;
        }
        nob_log(NOB_ERROR, "could not create directory `%s`: %s", path, strerror(errno));
        return false;
    }

    nob_log(NOB_INFO, "created directory `%s`", path);
    return true;
}

bool nob_copy_file(const std::string src_path, const std::string dst_path) {
    nob_log(NOB_INFO, "copying %s -> %s", src_path, dst_path);
#ifdef _WIN32
    if (!CopyFileW((LPCWSTR)src_path.c_str(), (LPCWSTR)dst_path.c_str(), FALSE)) {
        nob_log(NOB_ERROR, "Could not copy file: %lu", GetLastError());
        return false;
    }
    return true;
#else
    int src_fd = -1;
    int dst_fd = -1;
    size_t buf_size = 32 * 1024;
    auto buf = std::vector<char>(buf_size);
    NOB_ASSERT(buf.size() != 0 && "Buy more RAM lol!!");
    bool result = true;

    src_fd = open(src_path.c_str(), O_RDONLY);
    if (src_fd < 0) {
        nob_log(NOB_ERROR, "Could not open file %s: %s", src_path, strerror(errno));
        nob_return_defer(false);
    }

    struct stat src_stat;
    if (fstat(src_fd, &src_stat) < 0) {
        nob_log(NOB_ERROR, "Could not get mode of file %s: %s", src_path, strerror(errno));
        nob_return_defer(false);
    }

    dst_fd = open(dst_path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
    if (dst_fd < 0) {
        nob_log(NOB_ERROR, "Could not create file %s: %s", dst_path, strerror(errno));
        nob_return_defer(false);
    }

    for (;;) {
        ssize_t n = read(src_fd, buf.data(), buf_size);
        if (n == 0)
            break;
        if (n < 0) {
            nob_log(NOB_ERROR, "Could not read from file %s: %s", src_path, strerror(errno));
            nob_return_defer(false);
        }
        char *buf2 = buf.data();
        while (n > 0) {
            ssize_t m = write(dst_fd, buf2, n);
            if (m < 0) {
                nob_log(NOB_ERROR, "Could not write to file %s: %s", dst_path, strerror(errno));
                nob_return_defer(false);
            }
            n -= m;
            buf2 += m;
        }
    }

defer:
    close(src_fd);
    close(dst_fd);
    return result;
#endif
}

void nob_cmd_render(Nob_Cmd cmd, std::string &render) {
    for (size_t i = 0; i < cmd.size(); ++i) {
        const char *arg = cmd[i].c_str();
        if (arg == NULL)
            break;
        if (i > 0)
            render.push_back(' ');
        if (!strchr(arg, ' ')) {
            render.append(arg);
        } else {
            render.push_back('\'');
            render.append(arg);
            render.push_back('\'');
        }
    }
}

Nob_Proc nob_cmd_run_async(Nob_Cmd &cmd) {
    if (cmd.size() < 1) {
        nob_log(NOB_ERROR, "Could not run empty command");
        return NOB_INVALID_PROC;
    }

    std::string sb;

    nob_cmd_render(cmd, sb);
    nob_log(NOB_INFO, "CMD: %s", sb.c_str());

    sb.clear();

#ifdef _WIN32
    // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    // NOTE: theoretically setting NULL to std handles should not be a problem
    // https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
    // TODO: check for errors in GetStdHandle
    siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // TODO: use a more reliable rendering of the command instead of cmd_render
    // cmd_render is for logging primarily
    nob_cmd_render(cmd, sb);
    BOOL bSuccess = CreateProcessA(NULL, (LPSTR)sb.c_str(), NULL, NULL, TRUE, 0, NULL, NULL, (LPSTARTUPINFOA)&siStartInfo, &piProcInfo);

    if (!bSuccess) {
        nob_log(NOB_ERROR, "Could not create child process: %lu", GetLastError());
        return NOB_INVALID_PROC;
    }

    CloseHandle(piProcInfo.hThread);

    return piProcInfo.hProcess;
#else
    pid_t cpid = fork();
    if (cpid < 0) {
        nob_log(NOB_ERROR, "Could not fork child process: %s", strerror(errno));
        return NOB_INVALID_PROC;
    }

    if (cpid == 0) {
        // NOTE: This leaks a bit of memory in the child process.
        // But do we actually care? It's a one off leak anyway...
        std::vector<const char *> cmd_null;
        for (int i = 0; i < cmd.size(); ++i)
            cmd_null.push_back(cmd[i].c_str());
        cmd_null.push_back(NULL);
        if (execvp(cmd[0].c_str(), (char *const *)cmd_null.data()) < 0) {
            nob_log(NOB_ERROR, "%s", cmd[0].c_str());
            nob_log(NOB_ERROR, "Could not exec child process: %s", strerror(errno));
            exit(1);
        }
        NOB_ASSERT(0 && "unreachable");
    }

    return cpid;
#endif
}

bool nob_procs_wait(std::vector<Nob_Proc> procs) {
    bool success = true;
    for (size_t i = 0; i < procs.size(); ++i) {
        success = nob_proc_wait(procs[i]) && success;
    }
    return success;
}

bool nob_proc_wait(Nob_Proc proc) {
    if (proc == NOB_INVALID_PROC)
        return false;

#ifdef _WIN32
    DWORD result = WaitForSingleObject(
        proc,     // HANDLE hHandle,
        INFINITE  // DWORD  dwMilliseconds
    );

    if (result == WAIT_FAILED) {
        nob_log(NOB_ERROR, "could not wait on child process: %lu", GetLastError());
        return false;
    }

    DWORD exit_status;
    if (!GetExitCodeProcess(proc, &exit_status)) {
        nob_log(NOB_ERROR, "could not get process exit code: %lu", GetLastError());
        return false;
    }

    if (exit_status != 0) {
        nob_log(NOB_ERROR, "command exited with exit code %lu", exit_status);
        return false;
    }

    CloseHandle(proc);

    return true;
#else
    for (;;) {
        int wstatus = 0;
        if (waitpid(proc, &wstatus, 0) < 0) {
            nob_log(NOB_ERROR, "could not wait on command (pid %d): %s", proc, strerror(errno));
            return false;
        }

        if (WIFEXITED(wstatus)) {
            int exit_status = WEXITSTATUS(wstatus);
            if (exit_status != 0) {
                nob_log(NOB_ERROR, "command exited with exit code %d", exit_status);
                return false;
            }

            break;
        }

        if (WIFSIGNALED(wstatus)) {
            nob_log(NOB_ERROR, "command process was terminated by %s", strsignal(WTERMSIG(wstatus)));
            return false;
        }
    }

    return true;
#endif
}

bool nob_cmd_run_sync(Nob_Cmd &cmd) {
    Nob_Proc p = nob_cmd_run_async(cmd);
    if (p == NOB_INVALID_PROC)
        return false;
    return nob_proc_wait(p);
}

void nob_cmd_append_many(Nob_Cmd &cmd, ...) {
    va_list args;
    va_start(args, cmd);
    const char *str = va_arg(args, const char *);
    while (str != NULL) {
        cmd.push_back(str);
        str = va_arg(args, const char *);
    }
    va_end(args);
}

char *nob_shift_args(int *argc, char ***argv) {
    NOB_ASSERT(*argc > 0);
    char *result = **argv;
    (*argv) += 1;
    (*argc) -= 1;
    return result;
}

void nob_log(Nob_Log_Level level, const char *fmt, ...) {
    switch (level) {
        case NOB_INFO:
            fprintf(stderr, "[INFO] ");
            break;
        case NOB_WARNING:
            fprintf(stderr, "[WARNING] ");
            break;
        case NOB_ERROR:
            fprintf(stderr, "[ERROR] ");
            break;
        default:
            NOB_ASSERT(0 && "unreachable");
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

bool nob_read_entire_dir(const char *parent, std::vector<std::string> children) {
    bool result = true;
    DIR *dir = NULL;
    struct dirent *ent;

    dir = opendir(parent);
    if (dir == NULL) {
        nob_log(NOB_ERROR, "Could not open directory %s: %s", parent, strerror(errno));
        nob_return_defer(false);
    }

    errno = 0;
    ent = readdir(dir);
    while (ent != NULL) {
        children.push_back(nob_temp_strdup(ent->d_name));
        ent = readdir(dir);
    }

    if (errno != 0) {
        nob_log(NOB_ERROR, "Could not read directory %s: %s", parent, strerror(errno));
        nob_return_defer(false);
    }

defer:
    if (dir)
        closedir(dir);
    return result;
}

bool nob_write_entire_file(const char *path, void *data, size_t size) {
    bool result = true;

    FILE *f = fopen(path, "wb");
    char *buf;
    if (f == NULL) {
        nob_log(NOB_ERROR, "Could not open file %s for writing: %s\n", path, strerror(errno));
        nob_return_defer(false);
    }

    //           len
    //           v
    // aaaaaaaaaa
    //     ^
    //     data

    buf = (char *)data;
    while (size > 0) {
        size_t n = fwrite(buf, 1, size, f);
        if (ferror(f)) {
            nob_log(NOB_ERROR, "Could not write into file %s: %s\n", path, strerror(errno));
            nob_return_defer(false);
        }
        size -= n;
        buf += n;
    }

defer:
    if (f)
        fclose(f);
    return result;
}

Nob_File_Type nob_get_file_type(const std::string path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        nob_log(NOB_ERROR, "Could not get file attributes of %s: %lu", path, GetLastError());
        NOB_ASSERT(0 && "unreachable");
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return NOB_FILE_DIRECTORY;
    // TODO: detect symlinks on Windows (whatever that means on Windows anyway)
    return NOB_FILE_REGULAR;
#else   // _WIN32
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) < 0) {
        nob_log(NOB_ERROR, "Could not get stat of %s: %s", path.c_str(), strerror(errno));
        NOB_ASSERT(0 && "unreachable");
    }

    switch (statbuf.st_mode & S_IFMT) {
        case S_IFDIR:
            return NOB_FILE_DIRECTORY;
        case S_IFREG:
            return NOB_FILE_REGULAR;
        case S_IFLNK:
            return NOB_FILE_SYMLINK;
        default:
            return NOB_FILE_OTHER;
    }
#endif  // _WIN32
}

bool nob_copy_directory_recursively(std::string src_path, std::string dst_path) {
    bool result = true;
    std::vector<std::string> children;
    std::string src_sb;
    std::string dst_sb;
    size_t temp_checkpoint = nob_temp_save();

    Nob_File_Type type = nob_get_file_type(src_path);
    if (type < 0)
        return false;

    switch (type) {
        case NOB_FILE_DIRECTORY: {
            if (!nob_mkdir_if_not_exists(dst_path.c_str()))
                nob_return_defer(false);
            if (!nob_read_entire_dir(src_path.c_str(), children))
                nob_return_defer(false);

            for (size_t i = 0; i < children.size(); ++i) {
                if (strcmp(children[i].c_str(), ".") == 0)
                    continue;
                if (strcmp(children[i].c_str(), "..") == 0)
                    continue;

                src_sb.clear();
                src_sb.append(src_path);
                src_sb.append("/");
                src_sb.append(children[i]);

                dst_sb.clear();
                dst_sb.append(dst_path);
                dst_sb.append("/");
                dst_sb.append(children[i]);

                if (!nob_copy_directory_recursively(src_sb, dst_sb)) {
                    nob_return_defer(false);
                }
            }
        } break;

        case NOB_FILE_REGULAR: {
            if (!nob_copy_file(src_path, dst_path)) {
                nob_return_defer(false);
            }
        } break;

        case NOB_FILE_SYMLINK: {
            nob_log(NOB_WARNING, "TODO: Copying symlinks is not supported yet");
        } break;

        case NOB_FILE_OTHER: {
            nob_log(NOB_ERROR, "Unsupported type of file %s", src_path);
            nob_return_defer(false);
        } break;

        default:
            NOB_ASSERT(0 && "unreachable");
    }

defer:
    nob_temp_rewind(temp_checkpoint);
    return result;
}

char *nob_temp_strdup(const char *cstr) {
    size_t n = strlen(cstr);
    char *result = (char *)nob_temp_alloc(n + 1);
    NOB_ASSERT(result != NULL && "Increase NOB_TEMP_CAPACITY");
    memcpy(result, cstr, n);
    result[n] = '\0';
    return result;
}

void *nob_temp_alloc(size_t size) {
    if (nob_temp_size + size > NOB_TEMP_CAPACITY)
        return NULL;
    void *result = &nob_temp[nob_temp_size];
    nob_temp_size += size;
    return result;
}

char *nob_temp_sprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    int n = vsnprintf(NULL, 0, format, args);
    va_end(args);

    NOB_ASSERT(n >= 0);
    char *result = (char *)nob_temp_alloc(n + 1);
    NOB_ASSERT(result != NULL && "Extend the size of the temporary allocator");
    // TODO: use proper arenas for the temporary allocator;
    va_start(args, format);
    vsnprintf(result, n + 1, format, args);
    va_end(args);

    return result;
}

void nob_temp_reset(void) {
    nob_temp_size = 0;
}

size_t nob_temp_save(void) {
    return nob_temp_size;
}

void nob_temp_rewind(size_t checkpoint) {
    nob_temp_size = checkpoint;
}

const char *nob_temp_sv_to_cstr(Nob_String_View sv) {
    char *result = (char *)nob_temp_alloc(sv.count + 1);
    NOB_ASSERT(result != NULL && "Extend the size of the temporary allocator");
    memcpy(result, sv.data, sv.count);
    result[sv.count] = '\0';
    return result;
}

int nob_needs_rebuild(std::string output_path, std::vector<std::string> input_paths) {
    size_t input_paths_count = input_paths.size();
#ifdef _WIN32
    BOOL bSuccess;

    HANDLE output_path_fd = CreateFileW((LPWSTR)output_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if (output_path_fd == INVALID_HANDLE_VALUE) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (GetLastError() == ERROR_FILE_NOT_FOUND)
            return 1;
        nob_log(NOB_ERROR, "Could not open file %s: %lu", output_path, GetLastError());
        return -1;
    }
    FILETIME output_path_time;
    bSuccess = GetFileTime(output_path_fd, NULL, NULL, &output_path_time);
    CloseHandle(output_path_fd);
    if (!bSuccess) {
        nob_log(NOB_ERROR, "Could not get time of %s: %lu", output_path, GetLastError());
        return -1;
    }

    for (size_t i = 0; i < input_paths_count; ++i) {
        const char *input_path = input_paths[i];
        HANDLE input_path_fd = CreateFileW((LPWSTR)input_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
        if (input_path_fd == INVALID_HANDLE_VALUE) {
            // NOTE: non-existing input is an error cause it is needed for building in the first place
            nob_log(NOB_ERROR, "Could not open file %s: %lu", input_path, GetLastError());
            return -1;
        }
        FILETIME input_path_time;
        bSuccess = GetFileTime(input_path_fd, NULL, NULL, &input_path_time);
        CloseHandle(input_path_fd);
        if (!bSuccess) {
            nob_log(NOB_ERROR, "Could not get time of %s: %lu", input_path, GetLastError());
            return -1;
        }

        // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
        if (CompareFileTime(&input_path_time, &output_path_time) == 1)
            return 1;
    }

    return 0;
#else
    struct stat statbuf = {0};

    if (stat(output_path.c_str(), &statbuf) < 0) {
        // NOTE: if output does not exist it 100% must be rebuilt
        if (errno == ENOENT)
            return 1;
        nob_log(NOB_ERROR, "could not stat %s: %s", output_path, strerror(errno));
        return -1;
    }
    int output_path_time = statbuf.st_mtime;

    for (size_t i = 0; i < input_paths_count; ++i) {
        const char *input_path = input_paths[i].c_str();
        if (stat(input_path, &statbuf) < 0) {
            // NOTE: non-existing input is an error cause it is needed for building in the first place
            nob_log(NOB_ERROR, "could not stat %s: %s", input_path, strerror(errno));
            return -1;
        }
        int input_path_time = statbuf.st_mtime;
        // NOTE: if even a single input_path is fresher than output_path that's 100% rebuild
        if (input_path_time > output_path_time)
            return 1;
    }

    return 0;
#endif
}

int nob_needs_rebuild1(const char *output_path, const char *input_path) {
    return nob_needs_rebuild(output_path, {input_path});
}

bool nob_rename(const char *old_path, const char *new_path) {
    nob_log(NOB_INFO, "renaming %s -> %s", old_path, new_path);
#ifdef _WIN32
    if (!MoveFileExW((LPWSTR)old_path, (LPWSTR)new_path, MOVEFILE_REPLACE_EXISTING)) {
        nob_log(NOB_ERROR, "could not rename %s to %s: %lu", old_path, new_path, GetLastError());
        return false;
    }
#else
    if (rename(old_path, new_path) < 0) {
        nob_log(NOB_ERROR, "could not rename %s to %s: %s", old_path, new_path, strerror(errno));
        return false;
    }
#endif  // _WIN32
    return true;
}

bool nob_read_entire_file(const char *path, std::string &sb) {
    bool result = true;
    size_t buf_size = 32 * 1024;
    std::vector<uint8_t> buf(buf_size);
    FILE *f = fopen(path, "rb");
    size_t n;
    if (f == NULL) {
        nob_log(NOB_ERROR, "Could not open %s for reading: %s", path, strerror(errno));
        nob_return_defer(false);
    }

    n = fread(buf.data(), 1, buf_size, f);
    while (n > 0) {
        sb.append((char *)buf.data(), n);
        n = fread(buf.data(), 1, buf_size, f);
    }
    if (ferror(f)) {
        nob_log(NOB_ERROR, "Could not read %s: %s\n", path, strerror(errno));
        nob_return_defer(false);
    }

defer:
    if (f)
        fclose(f);
    return result;
}

Nob_String_View nob_sv_chop_by_delim(Nob_String_View *sv, char delim) {
    size_t i = 0;
    while (i < sv->count && sv->data[i] != delim) {
        i += 1;
    }

    Nob_String_View result = nob_sv_from_parts(sv->data, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data += i + 1;
    } else {
        sv->count -= i;
        sv->data += i;
    }

    return result;
}

Nob_String_View nob_sv_from_parts(const char *data, size_t count) {
    Nob_String_View sv;
    sv.count = count;
    sv.data = data;
    return sv;
}

Nob_String_View nob_sv_trim_left(Nob_String_View sv) {
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[i])) {
        i += 1;
    }

    return nob_sv_from_parts(sv.data + i, sv.count - i);
}

Nob_String_View nob_sv_trim_right(Nob_String_View sv) {
    size_t i = 0;
    while (i < sv.count && isspace(sv.data[sv.count - 1 - i])) {
        i += 1;
    }

    return nob_sv_from_parts(sv.data, sv.count - i);
}

Nob_String_View nob_sv_trim(Nob_String_View sv) {
    return nob_sv_trim_right(nob_sv_trim_left(sv));
}

Nob_String_View nob_sv_from_cstr(const char *cstr) {
    return nob_sv_from_parts(cstr, strlen(cstr));
}

bool nob_sv_eq(Nob_String_View a, Nob_String_View b) {
    if (a.count != b.count) {
        return false;
    } else {
        return memcmp(a.data, b.data, a.count) == 0;
    }
}

// RETURNS:
//  0 - file does not exists
//  1 - file exists
// -1 - error while checking if file exists. The error is logged
int nob_file_exists(const char *file_path) {
#if _WIN32
    // TODO: distinguish between "does not exists" and other errors
    DWORD dwAttrib = GetFileAttributesA(file_path);
    return dwAttrib != INVALID_FILE_ATTRIBUTES;
#else
    struct stat statbuf;
    if (stat(file_path, &statbuf) < 0) {
        if (errno == ENOENT)
            return 0;
        nob_log(NOB_ERROR, "Could not check if file %s exists: %s", file_path, strerror(errno));
        return -1;
    }
    return 1;
#endif
}

// minirent.h SOURCE BEGIN ////////////////////////////////////////
#ifdef _WIN32
struct DIR {
    HANDLE hFind;
    LPWIN32_FIND_DATAW data;
    struct dirent *dirent;
};

DIR *opendir(const char *dirpath) {
    assert(dirpath);

    char buffer[MAX_PATH];
    snprintf(buffer, MAX_PATH, "%s\\*", dirpath);

    DIR *dir = (DIR *)calloc(1, sizeof(DIR));

    dir->hFind = FindFirstFileW((LPCWSTR)buffer, dir->data);
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        // TODO: opendir should set errno accordingly on FindFirstFile fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        goto fail;
    }

    return dir;

fail:
    if (dir) {
        free(dir);
    }

    return NULL;
}

struct dirent *readdir(DIR *dirp) {
    assert(dirp);

    if (dirp->dirent == NULL) {
        dirp->dirent = (struct dirent *)calloc(1, sizeof(struct dirent));
    } else {
        if (!FindNextFileW(dirp->hFind, dirp->data)) {
            if (GetLastError() != ERROR_NO_MORE_FILES) {
                // TODO: readdir should set errno accordingly on FindNextFile fail
                // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
                errno = ENOSYS;
            }

            return NULL;
        }
    }

    memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));

    strncpy(
        dirp->dirent->d_name,
        (const char *)dirp->data->cFileName,
        sizeof(dirp->dirent->d_name) - 1);

    return dirp->dirent;
}

int closedir(DIR *dirp) {
    assert(dirp);

    if (!FindClose(dirp->hFind)) {
        // TODO: closedir should set errno accordingly on FindClose fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        return -1;
    }

    if (dirp->dirent) {
        free(dirp->dirent);
    }
    free(dirp);

    return 0;
}
#endif  // _WIN32
// minirent.h SOURCE END ////////////////////////////////////////

#endif
