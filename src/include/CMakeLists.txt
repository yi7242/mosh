# Generate configuration header
configure_file(
    ${CMAKE_SOURCE_DIR}/src/include/config.h.in
    ${CMAKE_BINARY_DIR}/src/include/config.h
)

# Create a simple config.h if config.h.in doesn't exist
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/src/include/config.h.in)
    file(WRITE ${CMAKE_BINARY_DIR}/src/include/config.h
"#ifndef CONFIG_H
#define CONFIG_H

/* Platform detection */
#ifdef _WIN32
#define HAVE_WINSOCK2_H 1
#define HAVE_WINDOWS_H 1
#else
#define HAVE_SYS_SOCKET_H 1
#define HAVE_NETINET_IN_H 1
#define HAVE_ARPA_INET_H 1
#define HAVE_UNISTD_H 1
#define HAVE_TERMIOS_H 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_PTY_H 1
#define HAVE_UTIL_H 1
#endif

/* OpenSSL AES-OCB support */
#define USE_AES_OCB_FROM_OPENSSL 0

/* Package information */
#define PACKAGE_VERSION \"${PROJECT_VERSION}\"
#define PACKAGE_STRING \"mosh ${PROJECT_VERSION}\"

#endif /* CONFIG_H */
")
endif()
