# Compiler
CXX = g++

# Compiler flags
# -std=c++17: Use C++17
# -Wall: Turn on all warnings
# -g: Include debug symbols
# -O2: Optimize for speed
CXXFLAGS = -std=c++17 -Wall -g -O2

# Project name
TARGET = ccomp

# Source files
# This finds all .cpp files in the root and in the includes/ subdirectories
SRCS = $(wildcard *.cpp) $(wildcard includes/file_utils/*.cpp) $(wildcard includes/system_utils/*.cpp)

# Object files (auto-generated from source files)
OBJS = $(SRCS:.cpp=.o)

# Include paths
# This tells the compiler to look for headers in these directories
CPPFLAGS = -I. -I./includes/argparse/include

# Install location
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin


# --- Main Targets ---

# Default target
all: $(TARGET)

# Link the program
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

# Compile .cpp files into .o (object) files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# --- Utility Targets ---

# Install the binary
install: all
	@echo "Installing $(TARGET) to $(BINDIR)..."
	@sudo install -Dm755 $(TARGET) $(BINDIR)/$(TARGET)
	@echo "Install complete."

# Uninstall the binary
uninstall:
	@echo "Removing $(TARGET) from $(BINDIR)..."
	@sudo rm -f $(BINDIR)/$(TARGET)
	@echo "Uninstall complete."

# Clean up object files and the binary
clean:
	@echo "Cleaning up build files..."
	@rm -f $(OBJS) $(TARGET)
	@echo "Clean complete."