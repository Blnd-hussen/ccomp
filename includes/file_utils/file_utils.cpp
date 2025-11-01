#include "./file_utils.hpp"

bool fileExists(const std::filesystem::path &filePath) {
  return std::filesystem::exists(filePath) &&
         std::filesystem::is_regular_file(filePath);
}

bool directoryExists(const std::filesystem::path &directoryPath) {
  return std::filesystem::exists(directoryPath) &&
         std::filesystem::is_directory(directoryPath);
}

std::string getRootDir() {
  return std::filesystem::current_path().string();
}
