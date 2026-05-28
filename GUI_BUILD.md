# GUI Build Guide

This document covers building and running the Qt6 graphical interface (`cpusim_gui`).

## Prerequisites

- **CMake** 3.16 or higher
- **C++17** compiler (g++, clang++, or MSVC)
- **Qt6** with **Core**, **Widgets**, and **Charts** modules

### Installing Qt6

**macOS:**

```bash
brew install qt6
export PATH="/opt/homebrew/opt/qt6/bin:$PATH"   # add to ~/.zshrc
```

**Linux (Ubuntu/Debian):**

```bash
sudo apt-get update
sudo apt-get install qt6-base-dev qt6-charts-dev cmake build-essential
```

**Windows:**

1. Install Qt6 from the [Qt Online Installer](https://www.qt.io/download-qt-installer) — enable **Qt Charts** for your kit.
2. Install CMake and a C++ toolchain (Visual Studio or MinGW).
3. Pass `CMAKE_PREFIX_PATH` pointing at your Qt installation when configuring.

## Build

From the project root:

```bash
# macOS / Linux
cmake -S . -B build -DBUILD_GUI=ON \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6 2>/dev/null || brew --prefix qt 2>/dev/null || echo "")"

cmake --build build -j
```

```powershell
# Windows (adjust Qt path)
cmake -S . -B build -DBUILD_GUI=ON -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64"
cmake --build build --config Release
```

To build only the GUI after a headless configure:

```bash
cmake --build build --target cpusim_gui
```

To build CLI and tests **without** the GUI:

```bash
cmake -S . -B build -DBUILD_GUI=OFF
cmake --build build -j
```

## Run

```bash
./build/cpusim_gui
```

On Windows with Visual Studio generators, the executable is often under `build/Release/cpusim_gui.exe`.

## GUI Quick Reference

1. **Open Program** or drag-and-drop a `.elf` or hex text file.
2. Select **Cache Configuration** and **Branch Predictor Configuration** before starting (changes apply on reset).
3. Use **Start**, **Step**, and **Reset** to control simulation.
4. Tabs: **Pipeline**, **Registers**, **Statistics**, **Memory**, **Dependencies**, **Program Output**.

See [README.md](README.md) for program loading, ELF cross-compilation, and troubleshooting.
