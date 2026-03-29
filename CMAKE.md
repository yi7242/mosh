# Building Mosh with CMake

This document describes how to build Mosh using CMake for cross-platform builds, including native Windows support.

## Prerequisites

### All Platforms
- CMake 3.15 or later
- C++17 compatible compiler
- Protocol Buffers (protobuf)
- OpenSSL
- zlib

### Platform-Specific

#### Windows
- **Visual Studio 2019 or later** (for native builds), OR
- **MinGW-w64** (for MSYS2/MinGW builds)
- **vcpkg** (recommended for dependency management)

#### Linux/Unix
- GCC 7+ or Clang 5+
- ncurses/curses library
- Optional: libutempter (for utmp entries)

#### macOS
- Xcode Command Line Tools or Xcode
- Homebrew (for dependencies)

## Quick Start

### Linux/macOS

```bash
# Install dependencies (example for Ubuntu)
sudo apt install build-essential cmake protobuf-compiler \
    libprotobuf-dev libssl-dev zlib1g-dev libncurses-dev

# Configure and build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run tests
cd build
ctest

# Install (optional)
sudo cmake --install build
```

### Windows with Visual Studio

```cmd
# Install vcpkg and dependencies
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
.\vcpkg install openssl:x64-windows protobuf:x64-windows zlib:x64-windows

# Configure
cmake -B build -G "Visual Studio 16 2019" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build
cmake --build build --config Release

# Run
.\build\src\frontend\Release\mosh-client.exe
```

### Windows with MinGW (MSYS2)

```bash
# Install MSYS2, then in MSYS2 MinGW64 shell:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-protobuf mingw-w64-x86_64-openssl \
    mingw-w64-x86_64-zlib mingw-w64-x86_64-ninja

# Configure and build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## CMake Options

### Build Options

- `BUILD_TESTS` (default: ON) - Build test programs
- `BUILD_EXAMPLES` (default: ON) - Build example programs
- `ENABLE_HARDENING` (default: ON) - Enable compiler hardening options (Unix only)
- `ENABLE_UTEMPTER` (default: ON) - Enable utempter support (Unix only)

Example:
```bash
cmake -B build -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF
```

### Build Types

- `Release` - Optimized build without debug information
- `Debug` - Debug build with symbols
- `RelWithDebInfo` - Optimized with debug symbols
- `MinSizeRel` - Optimized for size

Example:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

## Advanced Usage

### Custom Installation Prefix

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

### Cross-Compilation

For cross-compiling to Windows from Linux (using MinGW):

```bash
cmake -B build-win64 \
    -DCMAKE_TOOLCHAIN_FILE=/path/to/mingw-w64-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-win64
```

### Using Ninja Generator

For faster builds on all platforms:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Verbose Build Output

```bash
cmake --build build --verbose
# Or
cmake --build build -- VERBOSE=1
```

## IDE Integration

### Visual Studio

Open the generated `.sln` file in the build directory, or use VS Code with the CMake extension.

### CLion

CLion has built-in CMake support. Just open the project directory.

### VS Code

1. Install the CMake Tools extension
2. Open the project directory
3. Select a kit (compiler)
4. Build using the status bar or command palette

## Troubleshooting

### Protocol Buffers Not Found

Make sure `protoc` is in your PATH:
```bash
which protoc  # Unix
where protoc  # Windows
```

For vcpkg on Windows:
```cmd
.\vcpkg install protobuf:x64-windows
```

### OpenSSL Not Found

For vcpkg on Windows:
```cmd
.\vcpkg install openssl:x64-windows
```

For Homebrew on macOS:
```bash
brew install openssl
cmake -B build -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl
```

### Windows: ConPTY API Not Available

The ConPTY API requires Windows 10 Version 1809 (October 2018 Update) or later. Make sure you're running a compatible Windows version.

### Link Errors on Windows

Make sure you're using the correct architecture (x64) and that all dependencies are built for the same architecture:
```cmd
.\vcpkg install openssl:x64-windows protobuf:x64-windows zlib:x64-windows
cmake -B build -A x64 ...
```

## Building Individual Components

To build only specific targets:

```bash
# Build only mosh-client
cmake --build build --target mosh-client

# Build only tests
cmake --build build --target base64-test

# Build specific example
cmake --build build --target benchmark
```

## Running Tests

```bash
cd build
ctest --output-on-failure

# Or run specific tests
./src/tests/base64-test
./src/tests/encrypt-decrypt-test
```

## Packaging

### Creating a Distribution Package

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build
cpack
```

### Windows MSI Installer

(To be implemented - see WINDOWS.md for future enhancements)

## Comparison with Autotools

| Feature | Autotools | CMake |
|---------|-----------|-------|
| Windows Support | Limited (MinGW only) | Full (MSVC, MinGW) |
| Build Speed | Slower | Faster (especially with Ninja) |
| IDE Integration | Limited | Excellent |
| Cross-compilation | Complex | Straightforward |
| Maintenance | More complex | Simpler |

## More Information

- See [WINDOWS.md](WINDOWS.md) for Windows-specific details
- See [README.md](README.md) for general Mosh information
- CMake documentation: https://cmake.org/documentation/
- vcpkg documentation: https://vcpkg.io/
