#include <cstdlib>
#include <fstream>
#include <string>

#include "./ccomp.hpp"
#include "includes/file_utils/file_utils.hpp"
#include "includes/system_utils/system_utils.hpp"

/**
 * @brief Parses command line arguments and builds the ProgramConfig.
 */
std::optional<ProgramConfig> parse_args(int argc, char **argv) {
  argparse::ArgumentParser program("ccomp");
  program.add_description(
      "CCOMP is a command-line utility designed to automate the compilation "
      "and execution of C++ source files on Unix-like systems.");

  program.add_argument("sourceFilePath")
      .help("c++ source file to be processed.")
      .required();

  program.add_argument("compiler_flags")
      .help("Additional flags to pass to the compiler (e.g., -Wall, -g, "
            "-Iinclude).")
      .remaining();

  program.add_argument("-r", "--run").flag();
  program.add_argument("-rv", "--runValgrind").flag();
  program.add_argument("-o", "--output")
      .default_value(std::string("./out"))
      .required();

  program.add_argument("-c", "--compiler")
      .help("Specifies the preferred compiler (e.g., gnu-20, clang++, g++-12).")
      .default_value(std::string("g++"))
      .required();

  try {
    program.parse_args(argc, argv);

    ProgramConfig config;
    config.sourceFilePath = program.get<std::string>("sourceFilePath");

    if (!std::regex_match(config.sourceFilePath.string(),
                          SOURCE_FILE_PATH_REGEX)) {
      throw std::invalid_argument(program.get<std::string>("sourceFilePath") +
                                  " is not a valid cplusplus file.");
    }
    if (!fileExists(config.sourceFilePath)) {
      throw std::ios::failure(config.sourceFilePath.string() +
                              " could not be found.");
    }

    config.outputPath = program.get<std::string>("--output");
    config.outputFileName =
        config.sourceFilePath.filename().replace_extension("");
    config.run = program.get<bool>("--run");
    config.runValgrind = program.get<bool>("--runValgrind");

    try {
      config.extraCompilerFlags =
          program.get<std::vector<std::string>>("compiler_flags");
    } catch (const std::exception &e) {
    }

    const std::string compilerArg = program.get<std::string>("--compiler");
    if (std::regex_match(compilerArg, COMPILER_REGEX)) {
      const auto preferredCompiler =
          constructPreferredCompilerPath(compilerArg);
      if (!preferredCompiler) {
        throw std::runtime_error(
            "Invalid compiler format. Expected 'gnu-XX' or 'clang-XX'");
      }
      config.compilerPath = preferredCompiler.value();
    } else {
      config.compilerPath = compilerArg;
    }

    return config;

  } catch (const std::exception &e) {
    exitError(ErrorType::ARGUMENT_PARSING_ERROR, e.what());
    return std::nullopt;
  }
}

/**
 * @brief Checks if output directory exists and prompts user to create it.
 */
bool prepare_environment(const ProgramConfig &config) {
  if (!directoryExists(config.outputPath)) {
    while (true) {
      std::cout << "Create output directory " + config.outputPath.string() +
                       "/ [y,n]: ";
      std::string input;
      std::getline(std::cin, input);

      if (input == "y" || input == "Y") {
        fs::create_directory(config.outputPath);
        std::cout << "Output directory created.\n";
        return true;
      } else if (input == "n" || input == "N") {
        exitError(ErrorType::PROCESS_ABORTED, "Proccess aborted by user ");
        return false;
      } else {
        std::cout << "Invalid input. Please enter 'y' or 'n'.\n";
      }
    }
  }
  return true;
}

/**
 * @brief Builds the full compilation command string.
 */
std::string build_compile_command(const ProgramConfig &config) {

  std::string command =
      (config.compilerPath + " " + config.sourceFilePath.string() + " -o " +
       (config.outputPath / config.outputFileName).string() + " ");

  for (const auto &flag : config.extraCompilerFlags) {
    command += flag + " ";
  }

  const auto includePaths = ExtractHeaderSourcePairs(config.sourceFilePath);
  for (const auto &[hppPath, cppPath] : includePaths) {
    if (fileExists(cppPath)) {
      command += (cppPath.string() + " ");
    } else {
      throw std::ios::failure("Could not find required source file: " +
                              cppPath.string());
    }
  }
  return command;
}

/**
 * @brief Executes the compile command and (if successful) the run/valgrind
 * command.
 */
int execute_commands(const ProgramConfig &config,
                     const std::string &compileCommand) {

  // * Run compilation
  if (safeSystemCall(compileCommand) != 0) {
    return exitError(ErrorType::COMPILATION_FAIL, "Compilation Failed",
                     compileCommand);
  }

  // * Run execution (if requested)
  if (config.run || config.runValgrind) {
    std::string runCommand{};
    if (config.runValgrind) {
      runCommand = "valgrind ";
    }
    runCommand += (config.outputPath / config.outputFileName).string();

    if (safeSystemCall(runCommand) != 0) {
      return exitError(ErrorType::EXECUTION_FAIL, "Execution Failed",
                       runCommand);
    }
  }

  return true;
}

int main(int argc, char **argv) {

  auto config_opt = parse_args(argc, argv);
  if (!config_opt) {
    return static_cast<int>(ErrorType::ARGUMENT_PARSING_ERROR);
  }

  auto config = config_opt.value();
  if (!prepare_environment(config)) {
    return static_cast<int>(ErrorType::PROCESS_ABORTED);
  }

  std::string compile_command;
  try {
    compile_command = build_compile_command(config);
  } catch (const std::exception &e) {
    return exitError(ErrorType::FILE_IO_ERROR, e.what());
  }

  if (!execute_commands(config, compile_command)) {
    return 1;
  }

  return 0;
}

std::map<fs::path, fs::path>
ExtractHeaderSourcePairs(const fs::path &sourceFilePath) {
  // ... (Your Tier 2 refactored logic goes here)
  std::ifstream file(sourceFilePath);
  if (!file.is_open()) {
    throw std::runtime_error("Unable to open file: " + sourceFilePath.string());
  }

  // * Find all available .cpp files in the project ONCE.
  std::map<std::string, fs::path> availableSources;
  const auto rootDir = getRootDir(); // * Uses fs::current_path()

  for (const auto &entry : fs::recursive_directory_iterator(rootDir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
      availableSources[entry.path().filename().string()] = entry.path();
    }
  }

  std::map<fs::path, fs::path> headerToSourceMap;
  std::smatch match;
  std::string line;
  const std::string mainFileName = sourceFilePath.filename().string();

  while (std::getline(file, line)) {
    if (std::regex_match(line, match, HEADER_REGEX)) {
      fs::path headerFile = match[1].str();
      fs::path cppFile = headerFile.replace_extension("cpp");
      std::string cppFileName = cppFile.filename().string();

      if (cppFileName == mainFileName)
        continue;

      if (availableSources.count(cppFileName)) {
        fs::path fullHeaderPath = rootDir / headerFile;
        headerToSourceMap[fullHeaderPath.replace_extension("hpp")] =
            availableSources[cppFileName];
      }
    }
  }
  return headerToSourceMap;
}

std::vector<std::string> splitString(const std::string &str, char delimiter) {
  std::vector<std::string> result;
  std::stringstream ss(str);
  std::string token;

  while (std::getline(ss, token, delimiter)) {
    result.push_back(token);
  }

  return result;
}

int exitError(const ErrorType &errorType, const std::string &message,
              const std::string &source) {
  int errorCode = static_cast<int>(errorType);
  std::cerr << "\033[31merror\033[0m: fail code " << errorCode << '\n';
  std::cerr << "- " << message << '\n';
  std::cerr << (source.empty() ? "\n" : "- Source: " + source + '\n');

  return errorCode;
}

std::optional<std::string>
constructPreferredCompilerPath(const std::string &compilerName) {
  if (!std::regex_match(compilerName, COMPILER_REGEX)) {
    return std::nullopt;
  }

  const auto tokens = splitString(compilerName, '-');
  const std::string selectedCompiler = tokens[0] == "gnu" ? "g++" : "clang++";

  return constructCompilerPath(selectedCompiler, tokens[1]);
}

std::string constructCompilerPath(const std::string &compilerName,
                                  const std::string &compilerVersion) {
  return compilerName + " -std=c++" + compilerVersion;
}