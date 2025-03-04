# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -Iinclude
LDFLAGS = -lpthread # Add necessary libraries here

# Directories
SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = build

# Source files
COMMON_SRC_FILES = $(wildcard $(INCLUDE_DIR)/*.cpp)
COMMON_OBJ_FILES = $(patsubst $(INCLUDE_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(COMMON_SRC_FILES))

# Targets
SERVER_TARGET = server
CLIENT_TARGET = client

# Default target
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Compile the server executable
$(SERVER_TARGET): $(COMMON_OBJ_FILES) $(OBJ_DIR)/server.o
	$(CXX) $(CXXFLAGS) -o $@ $(COMMON_OBJ_FILES) $(OBJ_DIR)/server.o $(LDFLAGS)

# Compile the client executable
$(CLIENT_TARGET): $(COMMON_OBJ_FILES) $(OBJ_DIR)/client.o
	$(CXX) $(CXXFLAGS) -o $@ $(COMMON_OBJ_FILES) $(OBJ_DIR)/client.o $(LDFLAGS)

# Compile common object files
$(OBJ_DIR)/%.o: $(INCLUDE_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile server and client separately
$(OBJ_DIR)/server.o: $(SRC_DIR)/server.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/client.o: $(SRC_DIR)/client.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create the build directory
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up build files
clean:
	rm -rf $(OBJ_DIR) $(SERVER_TARGET) $(CLIENT_TARGET)

# Phony targets
.PHONY: all clean

