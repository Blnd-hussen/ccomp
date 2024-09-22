#include "./ccomp.hpp"

int main(int argc, char **argv) {

  if (argc < 2) {
    std::cout << "CCOMP\n";
    return 0;
  }

  std::string compilerPath{}; 
  const auto compilerName = systemCompiler();
  const auto compilerStandardVersion = systemCompilerVersion();

  // ! invalid compiler -> 1
  if (compilerName.has_value() && compilerStandardVersion.has_value()) {
    compilerPath = constructCompilerPath(
      compilerName.value(), std::to_string(compilerStandardVersion.value())
    );
  } else {
    std::cerr << "Error: Unknown compiler" << '\n';
    return 1;
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
          // ! invalid compiler -> 1
          std::cerr << "Error: Unknown Compiler " << argv[i + 1] << '\n';
          return 1;
        }
    }
  }

  // ! Invalud source path -> 2
  if (sourcePath.empty() || !fs::exists(sourcePath)) {
    std::cout << "\033[38;5;160mProcess terminated -- exit code 2\033[0m\n";
    std::cout << "-- Possible Causes:\n";
    std::cout << "\033[38;5;95m-- source file was not provided\n";
    std::cout << "-- source file does not exist\033[0m\n";

    return 2;
  }

  // extract source file name
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
        
        // ! proccess aborted by user -> 3 
        std::cerr << "Error: Operation aborted by user.\n";
        return 3;
      } else {
        std::cout << "Invalid input. Please enter 'y' or 'n'.\n";
      }
    }
  }

  // construct the base system command
  std::string command =
      (compilerPath + " " + sourcePath.string() + " -o " +
       outputPath.string() + "/" + sourceFileName + " ");

  std::vector<fs::path> includePaths;
  try {
    includePaths = ExtractIncludePaths(sourcePath);
  } catch (const std::exception &e) {

    // ! #include file not found -> 4
    std::cerr << "\033[38;5;160m" << e.what() << "\033[0m\n";
    std::cout << "-- Possible Causes:\n\033[38;5;95m";
    std::cout << "-- File might not be readable\n";
    std::cout << "-- File might not exist\033[0m\n";

    return 3;
  }

  for (const fs::path &path : includePaths) {
    if (path.filename() != sourcePath)
      command += (path.string() + " ");
  }

  // check for successfull compilation
  // ! command faild -> 5
  if (std::system(command.c_str()) != 0) {
    std::cerr << "Compilation failed \n";
    return 3;
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
      std::cerr << "Execution failed. -- exit code: " << result << std::endl;
      return result;
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
    throw std::runtime_error(filePath.string() + " could not be processed");

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

void printError(const ErrorCode errorType, const std::string &message, const std::string source) {
  int errorCode{};
  switch (errorType)
  {
    case ErrorCode::INVALID_COMPILER_PATH:
      errorCode = 1;
      break;
    case ErrorCode::INVALID_SOURCE_PATH:
      errorCode = 2;
      break;
    case ErrorCode::INVALUD_SYSTEM_COMMAND:
      errorCode = 3;
      break;
    case ErrorCode::PROCESS_ABORTED:
      errorCode = 4;
      break;
  }
  std::cerr << "Error: fail code " << errorCode << '\n';
  std::cerr << message << '\n';
  std::cerr << "Error Source: " << source << '\n';
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