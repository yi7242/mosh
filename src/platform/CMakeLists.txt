# Platform abstraction layer

# Platform-specific sources
if(WINDOWS_BUILD)
    set(PLATFORM_SOURCES
        windows/pty_windows.cc
        windows/signal_windows.cc
        windows/terminal_windows.cc
        windows/network_windows.cc
        windows/locale_windows.cc
    )
else()
    set(PLATFORM_SOURCES
        posix/pty_posix.cc
        posix/signal_posix.cc
        posix/terminal_posix.cc
        posix/network_posix.cc
        posix/locale_posix.cc
    )
endif()

# Platform abstraction library
add_library(platform STATIC
    ${PLATFORM_SOURCES}
    pty_wrapper.h
    signal_wrapper.h
    terminal_wrapper.h
    network_wrapper.h
    locale_wrapper.h
)

target_include_directories(platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/util
)

if(WINDOWS_BUILD)
    target_link_libraries(platform PUBLIC
        ws2_32
        winmm
    )
endif()
