#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include "./ccomp.hpp"
#include "includes/argparse/include/argparse/argparse.hpp"
#include "includes/file_utils/file_utils.hpp"
#include "includes/system_utils/system_utils.hpp"

int main(int argc, char **argv) {

  std::string compilerPath{}; 
  const auto compilerName = systemCompiler();
  const auto compilerStandardVersion = systemCompilerVersion();

  if (compilerName.has_value() && compilerStandardVersion.has_value()) {
    compilerPath = constructCompilerPath(
      compilerName.value(), std::to_string(compilerStandardVersion.value())
    );
  } else {
    return exitError(
          ErrorType::INVALID_COMPILER_PATH, 
          "Unknown and/or Invalid System compiler", 
          std::to_string(__cplusplus)
    );
  }

  argparse::ArgumentParser program("ccomp");

  program.add_description(std::string(
    "CCOMP is a command-line utility designed to automate the compilation and execution of C++ source files\n"
    "on Unix-like systems. the program uses p-ranav's argparse library to parse arguments and\n"
    "regular expressions to find the cpp files based on the include files.\n\n"

    "Note: The tool assumes that each header file (.hpp) follows the pattern: R\"(^\\s*#include\\s*\\\"([^\\\"]+)\"\\s*$)\"\n"
    "meaning the program will look for a corresponding C++ file within the project directory.\n"
    "For example, #include \"header.hpp\" is expected to have a matching header.cpp file."
  ));

  program.add_argument("sourceFilePath")
    .help("c++ source file to be processed.")
    .required();

  program.add_argument("-r", "--run")
    .help("Executes the compiled binary after successful compilation.")
    .flag();

  program.add_argument("-rv", "--runValgrind")
    .help("Executes the compiled binary under Valgrind memory debugger after successful compilation.")
    .flag();

  program.add_argument("-o", "--output")
    .help("Specifies the output directory for the compiled binary.")
    .default_value(std::string("./out"))
    .required();

  program.add_argument("-c", "--compiler")
    .help("Specifies the preferred compiler to use (e.g., gnu-20 or clang-20). regex: ^(gnu|clang)-[0-9]{2}$")
    .default_value(compilerPath)
    .required();
  

  fs::path sourceFilePath{};
  try {
    program.parse_args(argc, argv);

    sourceFilePath = program.get<std::string>("sourceFilePath");
    
    if (!std::regex_match(sourceFilePath.string(), SOURCE_FILE_PATH_REGEX)) {
      throw std::invalid_argument(program.get<std::string>("sourceFilePath") + " is not a valid cplusplus file.");
    }

    if (!fileExists(sourceFilePath)) {
      throw std::ios::failure(sourceFilePath.string() + " could not be found.");
    }
  }
  catch (const std::invalid_argument &e) {
    return exitError(
      ErrorType::INVALID_SOURCE_PATH,
      e.what()
    );
  }
  catch (const std::ios_base::failure &e) {
    return exitError(
      ErrorType::FILE_IO_ERROR, 
      e.what()
    );
  }
  catch (const std::exception &e) {
    return exitError(
      ErrorType::ARGUMENT_PARSING_ERROR, 
      e.what()
    );
  }


  const std::string sourceFileName = sourceFilePath.filename().replace_extension("");
  const fs::path outputPath = program.get<std::string>("--output");

  if (!directoryExists(outputPath)) {
    while (true) {
      std::cout << "Create output directory " + outputPath.string() + "/ [y,n]: ";
      std::string input;
      std::getline(std::cin, input);

      if (input == "y" || input == "Y") {
        fs::create_directory(outputPath);
        std::cout << "Output directory created. Binary can be found at "
                  << outputPath.string() << "/" << sourceFileName << '\n';
        break;
      } else if (input == "n" || input == "N") {    
        return exitError(
          ErrorType::PROCESS_ABORTED,
          "Proccess aborted by user "
        );
      } else {
        std::cout << "Invalid input. Please enter 'y' or 'n'.\n";
      }
    }
  }

  if (program.is_used("--compiler")) {
    const std::optional<std::string> preferredCompiler = constructPreferredCompilerPath(
      program.get<std::string>("--compiler")
    );

    if (!preferredCompiler.has_value()) {
      return exitError(
        ErrorType::INVALID_COMPILER_PATH,
        "The '-c' flag expects a compiler and compiler version in the following format: '^(gnu|clang)-[0-9]{2}$'"
      );
    }
    
    compilerPath = preferredCompiler.has_value() 
      ? preferredCompiler.value()
      : compilerPath;
  }

  std::string command =(
    compilerPath + " " + sourceFilePath.string() + " -o " 
    + outputPath.string() + "/" + sourceFileName + " "
  );

  try {
    const auto includePaths = ExtractHeaderSourcePairs(sourceFilePath);

    for (const auto &[hppPath, cppPath]: includePaths) {
      if (fileExists(hppPath)) {
        command += (cppPath.string() + " ");
      } else {
        throw std::ios::failure(hppPath.string() + " could not be found.");
      }
    }
  } 
  catch (const std::exception &e) {
    return exitError(
      ErrorType::FILE_IO_ERROR,
      e.what()
    );
  }

  if (safeSystemCall(command) != 0) {
    return exitError(
      ErrorType::COMPILATION_FAIL,
      "Compilation Failed",
      command
    );
  }

  if (program["--run"] == true || program["--runValgrind"] == true) {
    std::string runCommand{};
    if (program["--runValgrind"] == true) {
      runCommand = "valgrind ";
    }

    runCommand += outputPath.string() + "/" + sourceFileName;
    if (safeSystemCall(runCommand) != 0) {
      return exitError(
        ErrorType::EXECUTION_FAIL,
        "Execution Failed",
        runCommand
      );
    }
  }

  return 0;
}

std::map<fs::path, fs::path> ExtractHeaderSourcePairs(const fs::path &sourceFilePath) {
  std::ifstream file(sourceFilePath);
  if (!file.is_open()) {
    throw std::runtime_error("Unable to open file: " + sourceFilePath.string());
  }

  std::map<fs::path, fs::path> headerToSourceMap;
  std::smatch match;
  std::string line;
  const auto rootDir = getRootDir(sourceFilePath);

  while (std::getline(file, line)) {
    if (std::regex_match(line, match, HEADER_REGEX)) {
      fs::path headerFile = match[1].str();
      fs::path cppFile = headerFile.replace_extension("cpp");

      if (cppFile.filename() == sourceFilePath.filename()) {
        continue;
      }

      for (const auto &entry : fs::recursive_directory_iterator(rootDir)) {
        if (cppFile.filename() == entry.path().filename()) {
          fs::path fullHeaderPath = rootDir / headerFile;
          headerToSourceMap[fullHeaderPath.replace_extension("hpp")] = entry.path();
          break;
        }
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

int exitError(const ErrorType &errorType, const std::string &message, const std::string &source) {
  int errorCode = static_cast<int>(errorType);
  std::cerr << "\033[31merror\033[0m: fail code " << errorCode << '\n';
  std::cerr << "- " << message << '\n';
  std::cerr << (source.empty() ? "\n" : "- Source: " + source + '\n');

  return errorCode;
}

std::optional<std::string> constructPreferredCompilerPath(const std::string &compilerName) {
  if (!std::regex_match(compilerName, COMPILER_REGEX)) {
    return std::nullopt;
  }

  const auto tokens = splitString(compilerName, '-');
  const std::string selectedCompiler = tokens[0] == "gnu"
    ? "g++"
    : "clang++";

  return constructCompilerPath(selectedCompiler, tokens[1]);
}

std::string constructCompilerPath(const std::string &compilerName, const std::string &compilerVersion) {
  return compilerName + " -std=c++" + compilerVersion;
}