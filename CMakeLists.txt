# Builds all four simulations at once:
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build -j
# Each subfolder is also a standalone CMake project and can be built on its own.
cmake_minimum_required(VERSION 3.16)
project(emergent_complexity LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_subdirectory(elementary_ca)
add_subdirectory(game_of_life)
add_subdirectory(smooth_life)
add_subdirectory(boids)
