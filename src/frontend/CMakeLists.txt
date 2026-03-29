# Frontend binaries

# Common libraries
set(MOSH_LIBS
    moshstatesync
    moshnetwork
    moshterminal
    moshcrypto
    moshutil
    moshprotos
    platform
    ${PROTOBUF_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

if(WINDOWS_BUILD)
    list(APPEND MOSH_LIBS ws2_32 winmm)
else()
    list(APPEND MOSH_LIBS ${CURSES_LIBRARIES})
    if(ENABLE_UTEMPTER)
        find_library(UTEMPTER_LIBRARY utempter)
        if(UTEMPTER_LIBRARY)
            list(APPEND MOSH_LIBS ${UTEMPTER_LIBRARY})
            add_definitions(-DHAVE_UTEMPTER=1)
        endif()
    endif()
endif()

# mosh-client
add_executable(mosh-client
    mosh-client.cc
    stmclient.cc
    stmclient.h
    terminaloverlay.cc
    terminaloverlay.h
)

target_link_libraries(mosh-client PRIVATE
    ${MOSH_LIBS}
)

# mosh-server
add_executable(mosh-server
    mosh-server.cc
)

target_link_libraries(mosh-server PRIVATE
    ${MOSH_LIBS}
)

# Install targets
install(TARGETS mosh-client mosh-server
    RUNTIME DESTINATION bin
)
