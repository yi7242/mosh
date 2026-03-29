# Crypto library

# Determine OCB implementation
if(OPENSSL_VERSION VERSION_GREATER_EQUAL "3.2")
    set(USE_OPENSSL_OCB TRUE)
else()
    set(USE_OPENSSL_OCB FALSE)
endif()

if(USE_OPENSSL_OCB)
    set(OCB_SOURCE ocb_openssl.cc)
else()
    set(OCB_SOURCE ocb_internal.cc)
endif()

add_library(moshcrypto STATIC
    ${OCB_SOURCE}
    ae.h
    base64.cc
    base64.h
    byteorder.h
    crypto.cc
    crypto.h
    prng.h
)

target_include_directories(moshcrypto PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries(moshcrypto PUBLIC
    OpenSSL::SSL
    OpenSSL::Crypto
)
