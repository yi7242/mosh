# Utility library
add_library(moshutil STATIC
    locale_utils.cc
    locale_utils.h
    swrite.cc
    swrite.h
    dos_assert.h
    fatal_assert.h
    select.h
    select.cc
    timestamp.h
    timestamp.cc
    pty_compat.cc
    pty_compat.h
)

target_include_directories(moshutil PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/include
)

if(WINDOWS_BUILD)
    target_link_libraries(moshutil PUBLIC
        ws2_32
    )
endif()
