#include "./ccomp.hpp"
#include "argparse/include/argparse/argparse.hpp"

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
      throw std::invalid_argument(program.get<std::string>("sourceFilePath"));
    }

    if (!fs::exists(sourceFilePath)) {
      throw std::ios::failure(sourceFilePath.string() + " could not be found.");
    }
  }
  catch (const std::invalid_argument &e) {
    std::cerr << e.what() << '\n';
    std::exit(static_cast<int>(ErrorType::INVALID_SOURCE_PATH));
  }
  catch (const std::ios_base::failure &e) {
    std::cerr << e.what() << '\n';
    std::exit(static_cast<int>(ErrorType::FILE_IO_ERROR));
  }
  catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    std::exit(static_cast<int>(ErrorType::ARGUMENT_PARSING_ERROR));
  }


  const std::string sourceFileName = sourceFilePath.filename().replace_extension("");
  const fs::path outputPath = program.get<std::string>("--output");

  if (!fs::is_directory(outputPath)) {
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
      command += (cppPath.string() + " ");
    }
  } 
  catch (const std::runtime_error &re) {
    return exitError(
          ErrorType::FILE_IO_ERROR,
          re.what()
    );
  }

  std::cout << command << '\n';  

  if (std::system(command.c_str()) != 0) {
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
    if (std::system(runCommand.c_str()) != 0) {
      return exitError(
        ErrorType::EXECUTION_FAIL,
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
          headerToSourceMap[headerFile] = entry.path();
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

int exitError(const ErrorType errorType, const std::string &message, const std::string &source) {
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

std::string getRootDir(const fs::path &path) {
  fs::path rootDir{};
  fs::path tempPath = path;

  while (!tempPath.parent_path().empty()) {
    rootDir = tempPath.parent_path();
    tempPath = tempPath.parent_path();
  }

  if (rootDir.empty()) {
    rootDir = "./";
  }

  return rootDir;
}

std::string constructCompilerPath(const std::string &compilerName, const std::string &compilerVersion) {
  return compilerName + " -std=c++" + compilerVersion;
}