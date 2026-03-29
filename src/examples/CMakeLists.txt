# Example programs

# Common libraries for examples
set(EXAMPLE_LIBS
    moshterminal
    moshcrypto
    moshutil
    moshnetwork
    moshprotos
    ${PROTOBUF_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

if(NOT WINDOWS_BUILD)
    list(APPEND EXAMPLE_LIBS ${CURSES_LIBRARIES})
endif()

# benchmark
add_executable(benchmark benchmark.cc)
target_link_libraries(benchmark PRIVATE ${EXAMPLE_LIBS})

# encrypt
add_executable(encrypt encrypt.cc)
target_link_libraries(encrypt PRIVATE ${EXAMPLE_LIBS})

# decrypt
add_executable(decrypt decrypt.cc)
target_link_libraries(decrypt PRIVATE ${EXAMPLE_LIBS})

# parse
add_executable(parse parse.cc)
target_link_libraries(parse PRIVATE ${EXAMPLE_LIBS})

# termemu
add_executable(termemu termemu.cc)
target_link_libraries(termemu PRIVATE ${EXAMPLE_LIBS})

# ntester
add_executable(ntester ntester.cc)
target_link_libraries(ntester PRIVATE ${EXAMPLE_LIBS})
