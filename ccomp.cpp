#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

// regex definations
const std::regex COMPILER_REGEX("^(gnu|clang)-[0-9]{2}$");
const std::regex SOURCEPATH_REGEX("^.+\\.cpp$");

// Function to extract include paths from a C++ source file
std::vector<std::string> ExtractIncludePaths(const std::filesystem::path &filePath);

// Function to split string based on delimiter
std::vector<std::string> splitString(const std::string &str, char delimiter);

// Function to check if correct compiler and version is specified
std::string evaluatePreferredCompiler(const std::string &compiler = "");

// Function to check if a file exists
bool FileExists(const std::filesystem::path &filePath);

std::string suffixCpp(const std::string &str);

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cout << "\033[38;5;160mCCOMP\033[0m\n";
    std::cout << "\033[38;5;95m-- The program expects at least one argument, "
                 "which should be the path to a C++ file.\n  Optionally, you "
                 "can specify additional flags to control the behavior:\n\n";

    std::cout << "-- Flag: '-r' Runs the compiled binary after successful compilation\n\n";

    std::cout << "-- Flag: '-c' specify compiler/compiler-version eg. ccomp -c gnu-17\n\n";

    std::cout << "-- Flag: '-o' [output_path]: Specifies the output directory for the compiled binary. Default is './out'\n\n";

    std::cout << "-- Example usage: ccomp -r file.cpp -o /build\033[0m\n\n";

    return 1;
  }

  std::filesystem::path sourcePath{};
  std::filesystem::path outputPath = "./out";
  std::string preferredCompiler = evaluatePreferredCompiler();
  bool runBinary = false;
  bool runValgrind = false;

  bool skipArgument = false;
  for (int i = 1; i < argc; ++i)
  {
    if (skipArgument)
    {
      skipArgument = false;
      continue;
    }

    std::string currentArg{argv[i]};

    if (std::regex_match(currentArg, SOURCEPATH_REGEX) && sourcePath.empty())
    {
      sourcePath = currentArg;
    }
    else if (currentArg == "-r")
    {
      runBinary = true;
    }
    else if (currentArg == "-rv")
    {
      runValgrind = true;
    }
    else if (currentArg == "-c")
    {
      if (i + 1 < argc && std::regex_match(argv[i + 1], COMPILER_REGEX))
      {
        preferredCompiler = evaluatePreferredCompiler(argv[i + 1]);
        skipArgument = true;
      }
      else
      {
        std::cerr << "Error: Compiler flag -c requires a valid compiler as the next argument.\n";
        return 4;
      }
    }
    else if (currentArg == "-o")
    {
      if (i + 1 < argc)
      {
        outputPath = argv[i + 1];
        skipArgument = true;
      }
      else
      {
        std::cerr << "Error: Output flag -o requires a valid output directory path as the next argument.\n";
        return 4;
      }
    }
    else
    {
      std::cout << "Error: Invalid argument or flag: " << currentArg << '\n';
      return 4;
    }
  }

  // base file was not provided
  if (sourcePath.empty() || !FileExists(sourcePath))
  {
    std::cout << "\033[38;5;160mProcess terminated -- exit code 2\033[0m\n";
    std::cout << "-- Possible Causes:\n";
    std::cout << "\033[38;5;95m-- source file was not provided\n";
    std::cout << "-- source file does not exist\033[0m\n";

    return 2;
  }

  // extract source file name
  std::string sourceFileName = sourcePath.string().substr(0, sourcePath.string().find_last_of('.'));

  // check if output directory exists
  if (outputPath == "./out" && !FileExists(outputPath))
  {
    std::filesystem::create_directory(outputPath);
    std::cout << "Output directory created. binary can be found at " << outputPath.string() << "/" << sourceFileName << '\n';
  }

  // construct the base system command
  std::string command = (preferredCompiler + " " + sourcePath.string() + " -o " + outputPath.string() + "/" + sourceFileName + " ");

  // get -ld files from the base file
  std::vector<std::string> includePaths;
  try
  {
    includePaths = ExtractIncludePaths(sourcePath);
  }
  catch (const std::exception &e)
  {
    std::cerr << "\033[38;5;160m" << e.what() << "\033[0m\n";
    std::cout << "-- Possible Causes:\n\033[38;5;95m";
    std::cout << "-- File might not be readable\n";
    std::cout << "-- File might not exist\033[0m\n";

    return 3;
  }

  for (std::string &path : includePaths)
  {
    command += (path + " ");
  }

  // if run is true
  if (runValgrind || runBinary)
  {
    command += " && ";
    if (runValgrind)
    {
      command += "valgrind ";
    }
    command += outputPath.string() + "/" + sourceFileName;
  }

  // check for success
  int result = system(command.c_str());
  if (result != 0)
  {
    std::cout << "Compilation failed with error code: " << result << std::endl;
    return 3;
  }

  return 0;
}

std::vector<std::string> ExtractIncludePaths(const std::filesystem::path &filePath)
{
  // if the file was not found
  std::ifstream file(filePath);
  if (!file.is_open())
    throw std::runtime_error(filePath.string() + " could not be processed");

  // a list to hold the include paths
  std::vector<std::string> matches;
  std::smatch match{};

  std::string line{};
  std::regex re("^#include \"(.+)\"$");
  while (std::getline(file, line))
  {
    if (std::regex_match(line, match, re))
      matches.push_back(suffixCpp(match[1].str()));

    if (line.find("int main") != std::string::npos)
      break;
  }
  file.close();

  return matches;
}

std::vector<std::string> splitString(const std::string &str, char delimiter)
{
  std::vector<std::string> result;
  std::stringstream ss(str);
  std::string token;

  while (std::getline(ss, token, delimiter))
  {
    result.push_back(token);
  }

  return result;
}

std::string evaluatePreferredCompiler(const std::string &compiler)
{
  if (!std::regex_match(compiler, COMPILER_REGEX))
    return "clang++ -std=c++20";

  auto tokens = splitString(compiler, '-');
  if (tokens.size() != 2)
    return "clang++ -std=c++20";

  if (tokens[0] == "gnu")
  {
    return "g++-" + tokens[1];
  }
  return "clang++ -std=c++" + tokens[1];
}

bool FileExists(const std::filesystem::path &filePath)
{
  return std::filesystem::exists(filePath);
}

std::string suffixCpp(const std::string &str)
{
  std::string res = str.substr(0, str.find_last_of('.'));
  return res + ".cpp";
}
