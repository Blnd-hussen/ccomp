#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

// Function to extract include paths from a C++ source file
std::vector<std::string> ExtractIncludePaths(std::string &filePath);

// Function to check if a file exists
bool FileExists(const std::string &filePath);

std::string suffixCpp(const std::string &str);

int main(int argc, char **argv)
{

  if (argc < 2)
  {
    std::cout << "\033[38;5;160mCCOMP\033[0m\n";
    std::cout << "\033[38;5;95m-- The program expects at least one argument, "
                 "which should be the path to a C++ file.\n  Optionally, you "
                 "can specify additional flags to control the behavior:\n\n";

    std::cout << "-- Flag: '-r' Runs the compiled binary after successful "
                 "compilation\n\n";

    std::cout << "-- Flag: '-o' [output_path]: Specifies the output directory "
                 "for the compiled binary. Default is './out'\n\n";

    std::cout << "-- Example usage: ccomp -r file.cpp -o /build\033[0m\n\n";

    return 1;
  }

  std::string sourcePath{};
  std::string outputDirectory = "./out";
  bool runBinary = false;
  bool runValgrind = false;

  std::regex re("^.+\\.cpp$");
  for (int i = 1; i < argc; ++i)
  {
    std::string currentArg{argv[i]};
    if (std::regex_match(currentArg, re) && sourcePath.empty())
    {
      sourcePath = currentArg;
    }

    if (currentArg == "-r") runBinary = true;

    if (currentArg == "-rv") runValgrind = true;

    if (currentArg == "-o") outputDirectory = argv[i + 1];
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

  if (outputDirectory == "./out" && !FileExists(outputDirectory))
    std::filesystem::create_directory(outputDirectory);

  // construct the base system command
  std::string sourceFilename = sourcePath.substr(0, sourcePath.find_last_of('.'));
  std::string command =
          "g++ " + sourcePath + " -o " + outputDirectory + "/" + sourceFilename + " ";

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
    if (runValgrind)
      command += "&& valgrind " + outputDirectory + "/" + sourceFilename;
    else
      command += "&& " + outputDirectory + "/" + sourceFilename;
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

std::string suffixCpp(const std::string &str)
{
  std::string res = str.substr(0, str.find_last_of('.'));
  return res + ".cpp";
}

bool FileExists(const std::string &filePath)
{
  return std::filesystem::exists(filePath);
}

std::vector<std::string> ExtractIncludePaths(std::string &filePath)
{
  // if the file was not found
  std::ifstream file(filePath);
  if (!file.is_open())
    throw std::runtime_error(filePath + " could not be processed");

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
