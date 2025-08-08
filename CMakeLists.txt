cmake_minimum_required(VERSION 3.11)
project(loop_shooter)
set(PROJECT_FOLDER src)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message("Generated with build type: ${CMAKE_BUILD_TYPE}")

# Set flags for debug and release builds
add_compile_options(
  "-Wall"
  "$<$<CONFIG:DEBUG>:-O0;-g3;-DDEBUG=1>"
  "$<$<CONFIG:RELEASE>:-O2>"
)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)

# Dependencies
set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
  endif()
endif()

add_executable(${PROJECT_NAME} ${PROJECT_FOLDER}/game.c)
target_link_libraries(${PROJECT_NAME} raylib)

