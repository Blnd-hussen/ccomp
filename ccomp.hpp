#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// constants
const std::regex HEADER_REGEX(R"(^\s*#include\s*\"([^\"]+)\"\s*$)");
const std::regex COMPILER_REGEX("^(gnu|clang)-[0-9]{2}$");
const std::regex SOURCEPATH_REGEX("^.+\\.cpp$");


// Function declarations
std::vector<std::string> splitString(const std::string &str, char delimiter);
std::string evaluatePreferredCompiler(const std::string &compiler = "");
std::vector<fs::path> ExtractIncludePaths(const fs::path &filePath);
std::string suffixCpp(const std::string &str);
