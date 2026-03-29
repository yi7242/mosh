# Protocol buffer definitions
set(PROTO_FILES
    userinput.proto
    hostinput.proto
    transportinstruction.proto
)

# Generate protobuf sources
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Create library
add_library(moshprotos STATIC
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

target_include_directories(moshprotos PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
)

target_link_libraries(moshprotos PUBLIC
    ${PROTOBUF_LIBRARIES}
)

# Disable warnings for generated code
if(MSVC)
    target_compile_options(moshprotos PRIVATE /W0)
else()
    target_compile_options(moshprotos PRIVATE -w)
endif()
