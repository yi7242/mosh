cmake_minimum_required(VERSION 3.15)
project(mosh VERSION 1.4.0 LANGUAGES CXX C)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set C11 standard for C files
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build test programs" ON)
option(BUILD_EXAMPLES "Build example programs" ON)
option(ENABLE_HARDENING "Enable compiler hardening options" ON)
option(ENABLE_UTEMPTER "Enable utempter support (Unix only)" ON)

# Detect platform
if(WIN32)
    set(WINDOWS_BUILD TRUE)
    add_definitions(-D_WIN32_WINNT=0x0A00)  # Windows 10
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
else()
    set(WINDOWS_BUILD FALSE)
endif()

# Find required packages
find_package(Protobuf REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

if(NOT WIN32)
    # Unix-specific packages
    find_package(Curses REQUIRED)
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_BINARY_DIR}/src/include
    ${PROTOBUF_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
)

# Compiler flags
if(MSVC)
    # Visual Studio flags
    add_compile_options(/W4 /WX- /utf-8)
    add_compile_options(/wd4100 /wd4127 /wd4244 /wd4267 /wd4996)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
else()
    # GCC/Clang flags
    add_compile_options(-Wall)
    if(ENABLE_HARDENING)
        add_compile_options(-fstack-protector-all -D_FORTIFY_SOURCE=2)
    endif()
endif()

# Subdirectories
add_subdirectory(src)
