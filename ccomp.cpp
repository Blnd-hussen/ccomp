#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

std::vector<std::string> getIncludePaths(std::string filePath);
std::string suffixCpp(std::string str);

int main(int argc, char **argv)
{

  if (argc < 2)
  {
    std::cout << "\033[38;5;160mInvalid Usage -- exit code 1\033[0m\n";
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

  std::string baseFile{};
  std::string outputPath = "./out";
  bool run = false;

  std::regex re("^.+\\.cpp$");
  for (int i = 1; i < argc; ++i)
  {
    std::string arg{argv[i]};
    if (std::regex_match(arg, re) && baseFile.empty())
    {
      baseFile = arg;
    }

    if (arg == "-r") run = true;

    if (arg == "-o") outputPath = argv[i + 1];
  }

  // base file was not provided
  if (baseFile.empty())
  {
    std::cout << "\033[38;5;160mProcess terminated -- exit code 2\n";
    std::cout << "\033[38;5;95m-- C++ file was not provided\033[0m\n";

    return 2;
  }

  if (outputPath == "./out" && !std::filesystem::exists(outputPath))
    std::filesystem::create_directory(outputPath);

  // construct the base system command
  std::string baseFileName = baseFile.substr(0, baseFile.find_last_of('.'));
  std::string command =
      "g++ " + baseFile + " -o " + outputPath + "/" + baseFileName + " ";

  // get -ld files from the base file
  std::vector<std::string> includePaths;
  try
  {
    includePaths = getIncludePaths(baseFile);
  }
  catch (const std::exception &e)
  {
    std::cerr << "\033[38;5;160m" << e.what() << "\033[0m\n";
    std::cout << "-- Possible Causes:\n\033[38;5;95m";
    std::cout << "-- File might not exist\n";
    std::cout << "-- File might not be readable\n";
    std::cout << "-- File might not be a valid c++ file\033[0m\n";

    return 3;
  }

  for (std::string &path : includePaths)
  {
    command += (path + " ");
  }

  // if run is true
  if (run)
    command += "&& " + outputPath + "/" + baseFileName;

  int result = system(command.c_str());
  if (result != 0)
  {
    std::cout << "Compilation failed with error code: " << result << std::endl;
    return 3;
  }

  return 0;
}

std::string suffixCpp(std::string str)
{
  std::string res = str.substr(0, str.find_last_of('.'));

  return res + ".cpp";
}

std::vector<std::string> getIncludePaths(std::string filePath)
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