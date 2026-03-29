# Windows Terminal Support for Mosh

This document describes the Windows native port of Mosh to work with Windows Terminal.

## Overview

Mosh has been ported to run natively on Windows using Windows Terminal's ConPTY (Console Pseudo-Console) API and native Windows APIs. This eliminates the need for WSL (Windows Subsystem for Linux) to run Mosh on Windows.

## Platform Abstraction Layer

The port introduces a platform abstraction layer in `src/platform/` that provides cross-platform interfaces for:

### 1. PTY (Pseudo-Terminal) Operations (`pty_wrapper.h`)

**Windows Implementation** (`windows/pty_windows.cc`):
- Uses ConPTY API (Windows 10 1809+)
- Functions: `CreatePseudoConsole()`, `ResizePseudoConsole()`, `ClosePseudoConsole()`
- Creates processes with `CreateProcessW()` with ConPTY attached
- Manages I/O through pipes connected to ConPTY

**POSIX Implementation** (`posix/pty_posix.cc`):
- Wraps existing `pty_compat.h` functionality
- Uses `forkpty()` or platform-specific alternatives

### 2. Signal/Event Handling (`signal_wrapper.h`)

**Windows Implementation** (`windows/signal_windows.cc`):
- Maps POSIX signals to Windows events
- Uses `SetConsoleCtrlHandler()` for Ctrl+C, Ctrl+Break, etc.
- Implements window resize detection via polling thread
- Uses Windows events for synchronization

**POSIX Implementation** (`posix/signal_posix.cc`):
- Wraps POSIX signal handling
- Uses `signal()` and `sigaction()`

Signal Mapping:
- `SIGNAL_WINCH` → Window resize events (monitored via polling)
- `SIGNAL_INT` → Ctrl+C (CTRL_C_EVENT)
- `SIGNAL_TERM` → Ctrl+Break (CTRL_BREAK_EVENT)
- `SIGNAL_HUP` → Console close/logoff (CTRL_CLOSE_EVENT, etc.)

### 3. Terminal I/O (`terminal_wrapper.h`)

**Windows Implementation** (`windows/terminal_windows.cc`):
- Uses Windows Console API
- Functions: `GetConsoleMode()`, `SetConsoleMode()`, `GetConsoleScreenBufferInfo()`
- Enables virtual terminal processing for ANSI escape sequences
- Manages UTF-8 code page settings

**POSIX Implementation** (`posix/terminal_posix.cc`):
- Uses `termios` functions
- Functions: `tcgetattr()`, `tcsetattr()`, `cfmakeraw()`
- Uses `ioctl()` for window size operations

### 4. Network Initialization (`network_wrapper.h`)

**Windows Implementation** (`windows/network_windows.cc`):
- Initializes Winsock 2.2 with `WSAStartup()`
- Cleans up with `WSACleanup()`

**POSIX Implementation** (`posix/network_posix.cc`):
- No-op (no initialization needed)

### 5. Locale/UTF-8 Support (`locale_wrapper.h`)

**Windows Implementation** (`windows/locale_windows.cc`):
- Uses Windows code pages (CP_UTF8)
- Functions: `SetConsoleCP()`, `SetConsoleOutputCP()`, `GetConsoleCP()`
- Detects and sets UTF-8 code page

**POSIX Implementation** (`posix/locale_posix.cc`):
- Wraps existing `locale_utils.h` functionality
- Uses `setlocale()` and `nl_langinfo()`

## Requirements

### Windows Requirements:
- **Windows 10 Version 1809 (October 2018 Update) or later**
  - Required for ConPTY API support
- **Windows Terminal** (recommended) or Windows Console Host with VT support
- **Visual Studio 2019 or later** or MinGW-w64 with GCC 7+
- **Dependencies:**
  - OpenSSL (available via vcpkg or pre-built binaries)
  - Protocol Buffers (available via vcpkg)
  - zlib (available via vcpkg)

### Build Tools:
- **Visual Studio** with C++ development tools, OR
- **MinGW-w64** with MSYS2, OR
- **CMake** 3.15+ (for cross-platform builds)

## Building on Windows

### Option 1: Visual Studio (Recommended)

1. Install Visual Studio 2019 or later with C++ workload
2. Install dependencies via vcpkg:
   ```cmd
   vcpkg install openssl:x64-windows protobuf:x64-windows zlib:x64-windows
   ```

3. Generate Visual Studio solution files (requires CMake support - to be added)

4. Build using Visual Studio or MSBuild

### Option 2: MinGW-w64 with MSYS2

1. Install MSYS2 from https://www.msys2.org/

2. Install build tools and dependencies:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-protobuf \
             mingw-w64-x86_64-openssl mingw-w64-x86_64-zlib \
             autoconf automake make
   ```

3. Run autogen and configure:
   ```bash
   ./autogen.sh
   ./configure --host=x86_64-w64-mingw32
   make
   ```

### Option 3: CMake (Cross-platform)

CMake build support is planned for easier cross-platform building.

## Architecture Notes

### ConPTY Integration

The Windows implementation uses ConPTY to create a pseudo-console that:
- Emulates a terminal environment for console applications
- Handles VT (Virtual Terminal) escape sequences
- Provides a compatible interface with Unix PTYs

ConPTY Process Flow:
```
mosh-client/server
    ↓
CreatePseudoConsole() ← Input/Output Pipes
    ↓
CreateProcessW() with PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
    ↓
Shell process (cmd.exe, PowerShell, bash.exe)
```

### Signal Handling Approach

Since Windows doesn't have POSIX signals:
1. **Console Control Events** (Ctrl+C, Ctrl+Break) are caught via `SetConsoleCtrlHandler()`
2. **Window Resize** is detected by polling `GetConsoleScreenBufferInfo()` in a background thread
3. **Process Events** use Windows event objects for synchronization

### File Descriptor Emulation

Windows handles are mapped to pseudo file descriptors for ConPTY I/O:
- Internal map maintains handle-to-fd associations
- Starting from fd=1000 to avoid conflicts with stdin/stdout/stderr

### UTF-8 Support

Windows Terminal supports UTF-8 natively when:
- Console code page is set to CP_UTF8 (65001)
- Virtual Terminal Processing is enabled
- Input/Output modes are properly configured

## Known Limitations

1. **Windows 10 1809+ Only**: ConPTY API is not available on earlier Windows versions
2. **I/O Multiplexing**: Currently uses polling; future versions may use overlapped I/O
3. **Signal Compatibility**: Some POSIX signals (SIGPIPE, SIGCHLD, etc.) have limited or no Windows equivalents
4. **Window Resize**: Polling-based detection has 500ms latency (could be optimized)
5. **No Cygwin Support**: This port targets native Windows, not Cygwin

## Testing

To test the Windows port:

1. **Build Tests**: Ensure the code compiles on Windows
2. **Basic Functionality**:
   - Run `mosh-client` and `mosh-server` locally
   - Test terminal I/O and escape sequences
   - Verify window resize handling
   - Test Ctrl+C and Ctrl+Break signal handling

3. **Network Tests**:
   - Test UDP packet transmission
   - Verify encryption/decryption
   - Test connection roaming

4. **UTF-8 Tests**:
   - Test Unicode character display
   - Verify UTF-8 encoding/decoding

## Future Enhancements

1. **CMake Build System**: Add CMake support for easier Windows builds
2. **Overlapped I/O**: Replace polling with asynchronous I/O for better performance
3. **Native Window Events**: Use Console API events instead of polling for resize detection
4. **MSI Installer**: Create Windows installer package
5. **Windows Terminal Integration**: Add Windows Terminal-specific features
6. **Chocolatey Package**: Distribute via Chocolatey package manager

## Directory Structure

```
src/platform/
├── Makefile.am                  # Build configuration
├── pty_wrapper.h                # PTY interface
├── signal_wrapper.h             # Signal interface
├── terminal_wrapper.h           # Terminal I/O interface
├── network_wrapper.h            # Network initialization interface
├── locale_wrapper.h             # Locale/UTF-8 interface
├── windows/
│   ├── pty_windows.cc          # ConPTY implementation
│   ├── signal_windows.cc       # Windows event handling
│   ├── terminal_windows.cc     # Console API implementation
│   ├── network_windows.cc      # Winsock initialization
│   └── locale_windows.cc       # Code page handling
└── posix/
    ├── pty_posix.cc            # POSIX PTY wrapper
    ├── signal_posix.cc         # POSIX signal wrapper
    ├── terminal_posix.cc       # termios wrapper
    ├── network_posix.cc        # Network no-op
    └── locale_posix.cc         # POSIX locale wrapper
```

## Contributing

When contributing Windows-specific code:

1. **Test on Windows Terminal**: Ensure compatibility with Windows Terminal
2. **Maintain Abstraction**: Use the platform abstraction layer, don't add #ifdef in existing code
3. **UTF-8 Always**: Assume UTF-8 encoding throughout the codebase
4. **Error Handling**: Use GetLastError() on Windows, map to errno-style codes where possible
5. **Documentation**: Update this file with any Windows-specific changes

## References

- [ConPTY API Documentation](https://docs.microsoft.com/en-us/windows/console/creating-a-pseudoconsole-session)
- [Windows Console API](https://docs.microsoft.com/en-us/windows/console/console-functions)
- [Windows Terminal](https://github.com/microsoft/terminal)
- [Virtual Terminal Sequences](https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences)

## License

This Windows port maintains the same GNU GPLv3 license as the original Mosh project.
