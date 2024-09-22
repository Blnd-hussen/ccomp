#include "./ccomp.hpp"

int main(int argc, char **argv) {

  if (argc < 2) {
    std::cout << "CCOMP\n";
    return 0;
  }

  std::string compilerPath{}; 
  const auto compilerName = systemCompiler();
  const auto compilerStandardVersion = systemCompilerVersion();

  if (compilerName.has_value() && compilerStandardVersion.has_value()) {
    compilerPath = constructCompilerPath(
      compilerName.value(), std::to_string(compilerStandardVersion.value())
    );
  } else {
    return exitError(
          ErrorCode::INVALID_COMPILER_PATH, 
          "Unknown and/or Invalid System compiler", 
          std::to_string(__cplusplus)
    );
  }

  fs::path sourcePath{};
  fs::path outputPath = "./out";
  bool runBinary = false;
  bool runValgrind = false;
  bool skipArgument = false;

  for (int i = 1; i < argc; ++i) {
    if (skipArgument) {
      skipArgument = false;
      continue;
    }

    std::string currentArg{argv[i]};
    
    if (std::regex_match(currentArg, SOURCEPATH_REGEX) && sourcePath.empty()) {
      sourcePath = currentArg;
    } else if (currentArg == "-r") {
      runBinary = true;
    } else if (currentArg == "-rv") {
      runValgrind = true;
    } else if (currentArg == "-o" && i + 1 < argc) {
        outputPath = argv[i + 1];
        skipArgument = true;
    } else if (currentArg == "-c" && i + 1 < argc) {
        auto preferredCompiler = constructPreferredCompilerPath(argv[i + 1]);
        if (preferredCompiler.has_value()) {
          compilerPath = preferredCompiler.value();
          skipArgument = true;
        } else {
          return exitError(
                ErrorCode::INVALID_COMPILER_PATH,
                "The -c flag expects a valid compilerName-compilerVersion",
                argv[i + 1]
          );
        }
    }
  }

  if (sourcePath.empty() || !fs::exists(sourcePath)) {
    return exitError(
          ErrorCode::INVALID_SOURCE_PATH,
          "Source path was not provided or does not exist",
          (!sourcePath.empty() ? sourcePath : "")
    );
  }

  std::string sourceFileName =
      sourcePath.string().substr(0, sourcePath.string().find_last_of('.'));

  // check if output directory exists
  if (!fs::is_directory(outputPath)) {
    while (true) {
      std::cout << "Create output directory " + outputPath.string() +
                       "/ [y,n]: ";
      std::string input;
      std::getline(std::cin, input);

      if (input == "y" || input == "Y") {
        fs::create_directory(outputPath);
        std::cout << "Output directory created. Binary can be found at "
                  << outputPath.string() << "/" << sourceFileName << '\n';
        break;
      } else if (input == "n" || input == "N") {    
        return exitError(
                ErrorCode::PROCESS_ABORTED,
                "Proccess aborted by user "
        );
      } else {
        std::cout << "Invalid input. Please enter 'y' or 'n'.\n";
      }
    }
  }

  // construct the base system command
  std::string command =
      (compilerPath + " " + sourcePath.string() + " -o " +
       outputPath.string() + "/" + sourceFileName + " ");

  try {
    auto const includePaths = ExtractIncludePaths(sourcePath);

    for (const fs::path &path : includePaths) {
      if (path.filename() != sourcePath.filename())
        command += (path.string() + " ");
    }
  } catch (const std::runtime_error &re) {
    return exitError(
            ErrorCode::FILE_NOT_FOUND,
            re.what()
    );
  }

  // check for successfull compilation
  if (std::system(command.c_str()) != 0) {
    return exitError(
          ErrorCode::COMPILATION_FAIL,
          "Compilation Failed",
          command
    );
  }

  std::cout << "command: " << command;
 
  // if run is true
  if (runBinary || runValgrind) {
    std::string runCommand{};
    if (runValgrind) {
      runCommand = "valgrind ";
    }
    runCommand += outputPath.string() + "/" + sourceFileName;

    std::cout << ", runCommand: " << runCommand << '\n';

    int result = std::system(runCommand.c_str());
    if (result != 0) {
      return exitError(
        ErrorCode::EXECUTION_FAIL,
        "Execution Failed",
        runCommand
      );
    }
  }

  return 0;
}


std::optional<int> systemCompilerVersion() {
  enum CppStandard : long {
    CPP98 = 199711L,
    CPP11 = 201103L,
    CPP14 = 201402L,
    CPP17 = 201703L,
    CPP20 = 202002L
  };

  switch (__cplusplus) {
  case CPP98: return 98;  // C++98
  case CPP11: return 11;  // C++11
  case CPP14: return 14;  // C++14
  case CPP17: return 17;  // C++17
  case CPP20: return 20;  // C++20
  default: return std::nullopt;        
  }
}

std::optional<std::string> systemCompiler() {
#ifdef __clang__
  return "clang++";
#elif defined(__GNUC__)
  return "g++";
#else
  return std::nullopt;
#endif
}

std::string suffixCpp(const std::string &str) {
  std::string res = str.substr(0, str.find_last_of('.'));
  return res + ".cpp";
}

std::vector<fs::path> ExtractIncludePaths(const fs::path &filePath) {
  // check if file is readable
  std::ifstream file(filePath);
  if (!file.is_open())
    throw std::runtime_error("Read permission denied for " + filePath.string());

  // a list to hold the include paths
  std::vector<fs::path> matches;
  std::smatch match{};

  std::string line{};
  while (std::getline(file, line)) {
    if (std::regex_match(line, match, HEADER_REGEX)) {
      matches.push_back(suffixCpp(match[1].str()));
    }
  }
  file.close();

  return matches;
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

int exitError(const ErrorCode errorType, const std::string &message, const std::string &source) {
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