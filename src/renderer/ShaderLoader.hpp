#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace AstroGenesis {

inline std::string getExecutableDir() {
#if defined(_WIN32)
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buffer, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        std::string path(buffer, len);
        size_t lastSlash = path.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            return path.substr(0, lastSlash);
        }
    }
#endif
    return "";
}

inline std::string loadShaderSource(const std::string& filepath) {
    std::vector<std::string> candidates;
    candidates.push_back(filepath);
    candidates.push_back("../" + filepath);
    candidates.push_back("../../" + filepath);

    std::string exeDir = getExecutableDir();
    if (!exeDir.empty()) {
        candidates.push_back(exeDir + "/" + filepath);
        candidates.push_back(exeDir + "/../" + filepath);
        candidates.push_back(exeDir + "/../../" + filepath);
        candidates.push_back(exeDir + "/../../../" + filepath);
    }

    for (const auto& path : candidates) {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (file.is_open()) {
            std::ostringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }
    }

    std::cerr << "[ShaderLoader] Error: Failed to open shader file: " << filepath << std::endl;
    return "";
}

} // namespace AstroGenesis
