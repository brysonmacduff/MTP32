# ============================
# Top-level CMakeLists.txt
# ============================

cmake_minimum_required(VERSION 3.14)
project(MTP32 VERSION 1.0.0 LANGUAGES CXX)

# Use modern C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Allow user to choose static/shared
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Install to standard locations
include(GNUInstallDirs)

find_package(GTest REQUIRED)

# Add library subdirectory
add_subdirectory(lib)