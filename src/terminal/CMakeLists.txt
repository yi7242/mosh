# Terminal library
add_library(moshterminal STATIC
    parseraction.cc
    parseraction.h
    parser.cc
    parser.h
    parserstate.cc
    parserstatefamily.h
    parserstate.h
    parsertransition.h
    terminal.cc
    terminaldispatcher.cc
    terminaldispatcher.h
    terminaldisplay.cc
    terminaldisplayinit.cc
    terminaldisplay.h
    terminalframebuffer.cc
    terminalframebuffer.h
    terminalfunctions.cc
    terminal.h
    terminaluserinput.cc
    terminaluserinput.h
)

target_include_directories(moshterminal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/include
)

if(NOT WINDOWS_BUILD)
    target_link_libraries(moshterminal PUBLIC
        ${CURSES_LIBRARIES}
    )
    target_include_directories(moshterminal PUBLIC
        ${CURSES_INCLUDE_DIRS}
    )
endif()
