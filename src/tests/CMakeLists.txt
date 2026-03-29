# Test programs

# Common libraries for tests
set(TEST_LIBS
    moshterminal
    moshcrypto
    moshutil
    moshnetwork
    moshstatesync
    moshprotos
    platform
    ${PROTOBUF_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
)

if(WINDOWS_BUILD)
    list(APPEND TEST_LIBS ws2_32)
else()
    list(APPEND TEST_LIBS ${CURSES_LIBRARIES})
endif()

# Test utilities
add_library(test_utils STATIC
    test_utils.cc
    test_utils.h
)

# base64 test
add_executable(base64-test base64.cc base64_vector.h)
target_link_libraries(base64-test PRIVATE ${TEST_LIBS})

# encrypt-decrypt test
add_executable(encrypt-decrypt-test encrypt-decrypt.cc)
target_link_libraries(encrypt-decrypt-test PRIVATE ${TEST_LIBS})

# nonce-incr test
add_executable(nonce-incr-test nonce-incr.cc)
target_link_libraries(nonce-incr-test PRIVATE ${TEST_LIBS})

# ocb-aes test
add_executable(ocb-aes-test ocb-aes.cc)
target_link_libraries(ocb-aes-test PRIVATE ${TEST_LIBS})

# Add tests
enable_testing()
add_test(NAME base64 COMMAND base64-test)
add_test(NAME encrypt-decrypt COMMAND encrypt-decrypt-test)
add_test(NAME nonce-incr COMMAND nonce-incr-test)
add_test(NAME ocb-aes COMMAND ocb-aes-test)
