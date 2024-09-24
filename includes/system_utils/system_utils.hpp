#pragma once

#include <iostream>
#include <optional>

int safeSystemCall(const std::string &);
std::optional<std::string> systemCompiler();
std::optional<int> systemCompilerVersion();