#include "./system_utils.hpp"
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

std::optional<int> systemCompilerVersion() {
  enum CppStandard : long {
    CPP98 = 199711L,
    CPP11 = 201103L,
    CPP14 = 201402L,
    CPP17 = 201703L,
    CPP20 = 202002L
  };

  switch (__cplusplus) {
  case CPP98:
    return 98; // C++98
  case CPP11:
    return 11; // C++11
  case CPP14:
    return 14; // C++14
  case CPP17:
    return 17; // C++17
  case CPP20:
    return 20; // C++20
  default:
    return std::nullopt;
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
