# State sync library
add_library(moshstatesync STATIC
    completeterminal.cc
    completeterminal.h
    user.cc
    user.h
)

target_include_directories(moshstatesync PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/include
)

target_link_libraries(moshstatesync PUBLIC
    moshterminal
    moshprotos
)
