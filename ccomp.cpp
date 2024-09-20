#include "./ccomp.hpp"

int main(int argc, char **argv)
{
  if (argc < 2)
  {
    std::cout << "\033[38;5;160mCCOMP\033[0m\n";
    return 0;
  }

  fs::path sourcePath{};
  fs::path outputPath = "./out";
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

  // base file was not provided or is empty
  if (sourcePath.empty() || !fs::exists(sourcePath))
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
  if (!fs::is_directory(outputPath))
  {
    while (true)
    {
      std::cout << "Create output directory " + outputPath.string() + "/ [y,n]: ";
      std::string input;
      std::getline(std::cin, input);

      if (input == "y" || input == "Y")
      {
        fs::create_directory(outputPath);
        std::cout << "Output directory created. Binary can be found at " << outputPath.string() << "/" << sourceFileName << '\n';
        break;
      }
      else if (input == "n" || input == "N")
      {
        std::cerr << "Error: Operation aborted by user.\n";
        return 3;
      }
      else
      {
        std::cout << "Invalid input. Please enter 'y' or 'n'.\n";
      }
    }
  }

  // construct the base system command
  std::string command = (preferredCompiler + " " + sourcePath.string() + " -o " + outputPath.string() + "/" + sourceFileName + " ");

  // get -ld files from the base file
  std::vector<fs::path> includePaths;
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

  for (const fs::path &path : includePaths)
  {
    if (path.filename() != sourcePath)
      command += (path.string() + " ");
  }

  std::cout << command << '\n';

  // check for successfull compilation
  if (system(command.c_str()) != 0)
  {
    std::cout << "Compilation failed with error code: \n";
    return 3;
  }

  // if run is true
  if (runBinary || runValgrind)
  {
    std::string runCommand;
    if (runValgrind)
    {
      runCommand = "valgrind " + outputPath.string() + "/" + sourceFileName;
    }
    else
    {
      runCommand = outputPath.string() + "/" + sourceFileName;
    }

    int result = system(runCommand.c_str());
    if (result != 0)
    {
      std::cerr << "Execution failed. -- exit code: " << result << std::endl;
      return result;
    }
  }

  return 0;
}

std::vector<fs::path> ExtractIncludePaths(const fs::path &filePath)
{
  // check if file is readable
  std::ifstream file(filePath);
  if (!file.is_open())
    throw std::runtime_error(filePath.string() + " could not be processed");

  // a list to hold the include paths
  std::vector<fs::path> matches;
  std::smatch match{};

  std::string line{};
  while (std::getline(file, line))
  {
    if (std::regex_match(line, match, HEADER_REGEX))
    {
      matches.push_back(suffixCpp(match[1].str()));
    }
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

  if (compiler.empty())
  {
    return "clang++ -std=c++20";
  }

  auto tokens = splitString(compiler, '-');

  if (tokens.size() != 2)
  {
    return "clang++ -std=c++20";
  }

  if (tokens[0] == "gnu")
  {
    return "g++-" + tokens[1];
  }
  return "clang++ -std=c++" + tokens[1];
}

std::string suffixCpp(const std::string &str)
{
  std::string res = str.substr(0, str.find_last_of('.'));
  return res + ".cpp";
}
