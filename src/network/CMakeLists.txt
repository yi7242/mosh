# Network library
add_library(moshnetwork STATIC
    network.cc
    network.h
    networktransport-impl.h
    networktransport.h
    transportfragment.cc
    transportfragment.h
    transportsender-impl.h
    transportsender.h
    transportstate.h
    compressor.cc
    compressor.h
)

target_include_directories(moshnetwork PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries(moshnetwork PUBLIC
    moshprotos
    moshcrypto
    ZLIB::ZLIB
)

if(WINDOWS_BUILD)
    target_link_libraries(moshnetwork PUBLIC
        ws2_32
    )
endif()
