#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <regex>
#include <vector>

#include "includes/argparse/include/argparse/argparse.hpp"

namespace Constants {
inline const std::regex HEADER_REGEX(R"(^\s*#include\s*\"([^\"]+)\"\s*$)");
inline const std::regex COMPILER_REGEX("^(gnu|clang)-[0-9]{2}$");
inline const std::regex SOURCE_FILE_PATH_REGEX("^.+\\.cpp$");

inline const std::string DEFAULT_OUTPUT_PATH = "./out";
}; // namespace Constants

namespace fs = std::filesystem;
using namespace Constants;

enum class ErrorType {
  ARGUMENT_PARSING_ERROR = 1,
  INVALID_COMPILER_PATH,
  INVALID_SOURCE_PATH,
  PROCESS_ABORTED,
  FILE_IO_ERROR,
  COMPILATION_FAIL,
  EXECUTION_FAIL
};

struct ProgramConfig {
  fs::path sourceFilePath;
  fs::path outputPath;
  std::string outputFileName;
  std::string compilerPath;
  bool run;
  bool runValgrind;
  std::vector<std::string> extraCompilerFlags;
};

std::vector<std::string> splitString(const std::string &, char);
std::map<fs::path, fs::path> ExtractHeaderSourcePairs(const fs::path &);
int exitError(const ErrorType &, const std::string &, const std::string & = "");
std::optional<std::string> constructPreferredCompilerPath(const std::string &);
std::string constructCompilerPath(const std::string &, const std::string &);
std::optional<ProgramConfig> parse_args(int argc, char **argv);
bool prepare_environment(const ProgramConfig &config);
std::string build_compile_command(const ProgramConfig &config);
int execute_commands(const ProgramConfig &config,
                     const std::string &compileCommand);