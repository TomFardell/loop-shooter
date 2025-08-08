# Loop Shooter

Simple game made in C with [raylib](https://github.com/raysan5/raylib). Currently no artwork or sounds, as I am mainly just trying to learn games programming.

![screenshot](https://i.imgur.com/UkTbvJo.png)

## Features

- Shop with upgrades
- Window resizing (not that impressive but was actually quite hard to implement)
- Different enemy types (and easy to add more)

## How to run

The game can be built using [CMake](https://cmake.org/download/). There are Debug and Release builds. Note CMake will download and install raylib in your build directory.

- **Debug**
    - Generates debug information (`-g`) and sets `DEBUG` flag in the code
    - Press `B` during gameplay to show debug UI
    - Press `M` in shop to add $1000
    - Compiler optimisations disabled
- **Release**
    - Compiler optimisations set to level 2

To build the game, do (starting in the base directory):
```console
# Make a build directory to house CMake files and raylib installation
mkdir build
cd build

# Release build
cmake ..
cmake --build .

# Debug build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

Then just run the produced executable `loop_shooter`(`.exe`).

> [!Note]
> I'm not sure if this works with Visual Studio on Windows. To use GCC on Windows, add the `-G "MinGW Makefiles"` flag to the first CMake command.

