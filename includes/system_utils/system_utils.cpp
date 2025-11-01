#include "./system_utils.hpp"
#include <iostream>
#include <array>

int safeSystemCall(const std::string &command) {
  std::array<char, 128> buffer;
  std::string result{};

  FILE *pipe = popen(command.c_str(), "r");
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }

  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
    result += buffer.data();
  }

  int returnCode = pclose(pipe);
  std::cout << result;

  return returnCode;
}