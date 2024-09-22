#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Error codes
enum class ErrorCode : int {
  INVALID_COMPILER_PATH = 1,
  INVALID_SOURCE_PATH = 2,
  PROCESS_ABORTED = 3,
  FILE_NOT_FOUND = 4,
  COMPILATION_FAIL = 5,
  EXECUTION_FAIL = 6
};

// constants
inline const std::regex HEADER_REGEX(R"(^\s*#include\s*\"([^\"]+)\"\s*$)");
inline const std::regex COMPILER_REGEX("^(gnu|clang)-[0-9]{2}$");
inline const std::regex SOURCEPATH_REGEX("^.+\\.cpp$");

// Function declarations
std::vector<std::string> splitString(const std::string &str, char delimiter);
std::vector<fs::path> ExtractIncludePaths(const fs::path &filePath);
std::string suffixCpp(const std::string &str);
int exitError(const ErrorCode errorType, const std::string &message, const std::string &source = "");

std::optional<std::string> systemCompiler();
std::optional<int> systemCompilerVersion();
std::optional<std::string>
constructPreferredCompilerPath(const std::string &compilerName);
std::string constructCompilerPath(const std::string &compilerName,
                                  const std::string &compilerVersion);