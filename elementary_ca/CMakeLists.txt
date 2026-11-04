cmake_minimum_required(VERSION 3.16)
project(elementary_ca LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

include(FetchContent)
set(RAYLIB_VERSION 5.5)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG ${RAYLIB_VERSION}
  GIT_SHALLOW TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_executable(elementary_ca
  src/main.cpp
)

target_link_libraries(elementary_ca PRIVATE raylib)
