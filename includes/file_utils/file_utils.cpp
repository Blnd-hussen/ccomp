#include "./file_utils.hpp"

bool fileExists(const std::filesystem::path &filePath) {
  return std::filesystem::exists(filePath) &&
         std::filesystem::is_regular_file(filePath);
}

bool directoryExists(const std::filesystem::path &directoryPath) {
  return std::filesystem::exists(directoryPath) &&
         std::filesystem::is_directory(directoryPath);
}

std::string getRootDir(const std::filesystem::path &path) {
  std::filesystem::path rootDir{};
  std::filesystem::path tempPath = path;

  while (!tempPath.parent_path().empty()) {
    rootDir = tempPath.parent_path();
    tempPath = tempPath.parent_path();
  }

  if (rootDir.empty()) {
    rootDir = "./";
  }

  return rootDir;
}
