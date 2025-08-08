cmake_minimum_required(VERSION 3.11)
project(game)

set(project_folder src)
set(CMAKE_CXX_FLAGS_DEBUG_INIT, )

# Set flags 
add_compile_options(
  "-Wall"
  "$<$<CONFIG:DEBUG>:-O0;-g3;-DDEBUG=1>"
  "$<$<CONFIG:RELEASE>:-O2;-DNDEBUG>"
)

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

add_executable(${PROJECT_NAME} src/game.c)
target_link_libraries(${PROJECT_NAME} raylib)

