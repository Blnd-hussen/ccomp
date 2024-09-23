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
#include <map>

namespace fs = std::filesystem;

// Error codes
enum class ErrorType {
  ARGUMENT_PARSING_ERROR = 1,
  INVALID_COMPILER_PATH,
  INVALID_SOURCE_PATH,
  PROCESS_ABORTED,
  FILE_IO_ERROR,
  COMPILATION_FAIL,
  EXECUTION_FAIL
};

// constants
inline const std::regex HEADER_REGEX(R"(^\s*#include\s*\"([^\"]+)\"\s*$)");
inline const std::regex COMPILER_REGEX("^(gnu|clang)-[0-9]{2}$");
inline const std::regex SOURCE_FILE_PATH_REGEX("^.+\\.cpp$");

// Function declarations
std::vector<std::string> splitString(const std::string &str, char delimiter);
std::map<fs::path, fs::path> ExtractHeaderSourcePairs(const fs::path &sourceFilePath);
std::string suffixCpp(const std::string &str);
std::string getRootDir(const fs::path &path);
int exitError(const ErrorType errorType, const std::string &message, const std::string &source = "");


std::optional<std::string> systemCompiler();
std::optional<int> systemCompilerVersion();
std::optional<std::string>
constructPreferredCompilerPath(const std::string &compilerName);
std::string constructCompilerPath(const std::string &compilerName,
                                  const std::string &compilerVersion);