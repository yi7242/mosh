/*
    Mosh: the mobile shell
    Copyright 2012 Keith Winstein

    Windows compatibility header for POSIX types and functions.
*/

#ifndef WINDOWS_COMPAT_H
#define WINDOWS_COMPAT_H

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <io.h>
#include <direct.h>
#include <process.h>

/* POSIX type aliases */
typedef int pid_t;
typedef int ssize_t;
typedef unsigned int mode_t;
typedef int uid_t;
typedef int gid_t;

/* File descriptor operations */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* Socket compatibility */
#define close(fd) closesocket(fd)
#define ioctl(fd, request, ...) ioctlsocket(fd, request, __VA_ARGS__)

/* Error codes */
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif
#ifndef ECONNRESET
#define ECONNRESET WSAECONNRESET
#endif

/* Process operations */
#define getpid _getpid

/* Directory operations */
#define mkdir(path, mode) _mkdir(path)
#define rmdir _rmdir
#define chdir _chdir
#define getcwd _getcwd

/* Path separator */
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

/* Sleep function (Windows uses milliseconds) */
#define sleep(seconds) Sleep((seconds) * 1000)
#define usleep(microseconds) Sleep((microseconds) / 1000)

/* Random number generation */
#ifndef HAVE_ARC4RANDOM
#define arc4random() ((unsigned int)rand())
#define arc4random_buf(buf, n) do { \
    unsigned char *p = (unsigned char *)(buf); \
    size_t i; \
    for (i = 0; i < (n); i++) { \
        p[i] = (unsigned char)rand(); \
    } \
} while(0)
#endif

/* String functions */
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define strdup _strdup

/* File operations */
#define unlink _unlink
#define fileno _fileno

/* Inline keyword */
#ifndef inline
#define inline __inline
#endif

#endif /* _WIN32 */

#endif /* WINDOWS_COMPAT_H */
