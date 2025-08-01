# --- Basic Project Setup ---
# Require a modern version of CMake. 3.16 is a good baseline for C++20.
cmake_minimum_required(VERSION 3.16)

# Define the project name, version, and language.
project(MyRaylibGame VERSION 1.0 LANGUAGES CXX)

# --- C++ Standard Configuration ---
# Set the C++ standard to C++20 for the entire project.
set(CMAKE_CXX_STANDARD 20)
# Enforce the standard, so CMake will fail if the compiler doesn't support it.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Prefer -std=c++20 over -gnu++20 to avoid compiler-specific extensions.
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Add Raylib Submodule ---
# Before adding the raylib subdirectory, we can set options to control its build.
# We don't need to build its examples or games, which saves compile time.
set(BUILD_EXAMPLES OFF CACHE BOOL "Disable building raylib examples" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "Disable building raylib games" FORCE)

# Add the raylib CMake project from the submodule directory.
# This will create the 'raylib' library target for us to use.
add_subdirectory(External/Submodules/raylib)
add_subdirectory(External/Submodules/kcp)

# --- Define Your Executable ---
# Create an executable target named 'my_game' from your source file(s).
add_executable(my_game src/main.cpp src/network.cpp)

# --- Link Libraries ---
# Link your executable against the raylib library.
# Modern CMake's target_link_libraries will automatically handle:
#   - Include directories (so #include "raylib.h" works)
#   - Linking the raylib library itself
#   - Linking any system libraries raylib depends on (like OpenGL, etc.)
#
# Using 'PRIVATE' means that raylib is a dependency for building 'my_game',
# but it is not a public dependency that other targets linking to 'my_game'
# would inherit. For an executable, PRIVATE is almost always correct.
target_link_libraries(my_game PRIVATE raylib PRIVATE kcp)

# --- (Optional) Set Output Directory ---
# This is a nice quality-of-life improvement to place the final executable
# in a 'bin' directory inside the build folder.
set_target_properties(my_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
