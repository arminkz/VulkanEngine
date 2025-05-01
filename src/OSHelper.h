#pragma once
#include "stdafx.h"


namespace OSHelper {

#if defined(_WIN32)
    #include <windows.h>
    std::string getExecutableDir() {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string fullPath(path);
        size_t pos = fullPath.find_last_of("\\/");
        return (pos != std::string::npos) ? fullPath.substr(0, pos + 1) : "";
    }
#elif defined(__linux__) || defined(__APPLE__)
    #include <unistd.h>
    #include <limits.h>
    std::string getExecutableDir() {
        char path[PATH_MAX];
        ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
        std::string fullPath(path, (count > 0) ? count : 0);
        size_t pos = fullPath.find_last_of('/');
        return (pos != std::string::npos) ? fullPath.substr(0, pos + 1) : "";
    }
#else
    #error "Unsupported platform"
#endif

}