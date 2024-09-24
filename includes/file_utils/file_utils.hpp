#pragma once

#include <filesystem>
#include <iostream>

std::string getRootDir(const std::filesystem::path &);
bool fileExists(const std::filesystem::path &);
bool directoryExists(const std::filesystem::path &);