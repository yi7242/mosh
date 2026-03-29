/*
    Mosh: the mobile shell
    Copyright 2012 Keith Winstein

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    In addition, as a special exception, the copyright holders give
    permission to link the code of portions of this program with the
    OpenSSL library under certain conditions as described in each
    individual source file, and distribute linked combinations including
    the two.

    You must obey the GNU General Public License in all respects for all
    of the code used other than OpenSSL. If you modify file(s) with this
    exception, you may extend this exception to your version of the
    file(s), but you are not obligated to do so. If you do not wish to do
    so, delete this exception statement from your version. If you delete
    this exception statement from all source files in the program, then
    also delete it here.
*/

/*
 * Windows compatibility shims for native (MSVC / clang-cl) Windows builds.
 *
 * Include this header first in every translation unit that needs POSIX-like
 * APIs on Windows.  It is a no-op on all other platforms.
 *
 * Coverage:
 *   - Winsock2 / ws2tcpip (replaces sys/socket.h, netinet/in.h, netdb.h)
 *   - Missing POSIX types  (ssize_t, socklen_t, sa_family_t)
 *   - Socket-close shim    (closesocket instead of close for SOCKETs)
 *   - Byte-order macros    (htobe16/be16toh/htobe64/be64toh)
 *   - errno mapping from WSA errors
 *   - timespec             (available since Win10 SDK 10.0.15063 but guarded)
 *   - Signal number stubs  (SIGWINCH, SIGCONT, SIGPIPE, SIGHUP)
 *   - stdin/stdout in binary / VT mode
 *   - Console-size query
 */

#ifndef WINDOWS_COMPAT_HPP
#define WINDOWS_COMPAT_HPP

#ifdef _WIN32

/* ------------------------------------------------------------------ */
/* Must come before any windows.h inclusion to avoid winsock.h clash  */
/* ------------------------------------------------------------------ */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00 /* Windows 10 */
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <bcrypt.h>    /* BCryptGenRandom */
#include <io.h>        /* _read, _write, _setmode */
#include <fcntl.h>     /* _O_BINARY */
#include <process.h>   /* _getpid */

#include <cerrno>
#include <cstdint>
#include <cstring>   /* strerror */

/* ------------------------------------------------------------------ */
/* POSIX integer types missing from MSVC                               */
/* ------------------------------------------------------------------ */
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif
#endif

#ifndef _SOCKLEN_T
#define _SOCKLEN_T
typedef int socklen_t;
#endif

typedef unsigned short sa_family_t;

/* ------------------------------------------------------------------ */
/* POSIX signal numbers that are missing / different on Windows        */
/* ------------------------------------------------------------------ */
#ifndef SIGWINCH
#define SIGWINCH 28
#endif
#ifndef SIGHUP
#define SIGHUP 1
#endif
#ifndef SIGPIPE
#define SIGPIPE 13
#endif
#ifndef SIGCONT
#define SIGCONT 18
#endif

/* ------------------------------------------------------------------ */
/* Socket close                                                        */
/* On Windows sockets are not file descriptors; use closesocket().    */
/* We provide an inline wrapper so code that calls close(sock_fd)     */
/* compiles cleanly.  Code that closes regular files must call        */
/* _close() / fclose() directly.                                      */
/* ------------------------------------------------------------------ */
inline int socket_close( SOCKET s )
{
  return closesocket( s );
}

/* ------------------------------------------------------------------ */
/* Errno mapping from Winsock last-error                               */
/* ------------------------------------------------------------------ */
inline void wsa_set_errno( void )
{
  switch ( WSAGetLastError() ) {
    case WSAEWOULDBLOCK:
      errno = EAGAIN;
      break;
    case WSAECONNRESET:
      errno = ECONNRESET;
      break;
    case WSAETIMEDOUT:
      errno = ETIMEDOUT;
      break;
    case WSAEINPROGRESS:
      errno = EINPROGRESS;
      break;
    case WSAEHOSTUNREACH:
      errno = EHOSTUNREACH;
      break;
    case WSAENETUNREACH:
      errno = ENETUNREACH;
      break;
    case WSAEADDRINUSE:
      errno = EADDRINUSE;
      break;
    case WSAEINVAL:
      errno = EINVAL;
      break;
    case WSAENOTSOCK:
      errno = ENOTSOCK;
      break;
    default:
      errno = EIO;
      break;
  }
}

/* ------------------------------------------------------------------ */
/* Byte-order macros                                                   */
/* Winsock provides htons/ntohs/htonl/ntohl; add 64-bit variants.    */
/* ------------------------------------------------------------------ */
#ifndef htobe16
#define htobe16( x ) htons( x )
#endif
#ifndef be16toh
#define be16toh( x ) ntohs( x )
#endif
#ifndef htobe32
#define htobe32( x ) htonl( x )
#endif
#ifndef be32toh
#define be32toh( x ) ntohl( x )
#endif

#ifndef htobe64
inline uint64_t htobe64( uint64_t x )
{
  return ( static_cast<uint64_t>( htonl( static_cast<uint32_t>( x >> 32 ) ) ) )
         | ( static_cast<uint64_t>( htonl( static_cast<uint32_t>( x & 0xFFFFFFFFULL ) ) ) << 32 );
}
#endif
#ifndef be64toh
inline uint64_t be64toh( uint64_t x )
{
  return htobe64( x );
}
#endif

/* ------------------------------------------------------------------ */
/* timespec — guaranteed available in Windows 10 SDK 10.0.15063+      */
/* Define a fallback for older SDKs just in case.                     */
/* ------------------------------------------------------------------ */
#if !defined( _TIMESPEC_DEFINED ) && !defined( HAVE_STRUCT_TIMESPEC )
#define _TIMESPEC_DEFINED
struct timespec
{
  long tv_sec;
  long tv_nsec;
};
#endif

/* ------------------------------------------------------------------ */
/* getpid shim                                                         */
/* ------------------------------------------------------------------ */
#ifndef getpid
#define getpid _getpid
#endif

/* ------------------------------------------------------------------ */
/* Winsock2 initialisation / teardown helpers                         */
/* ------------------------------------------------------------------ */
inline bool winsock_init( void )
{
  WSADATA wsa_data;
  int err = WSAStartup( MAKEWORD( 2, 2 ), &wsa_data );
  return ( err == 0 );
}

inline void winsock_cleanup( void )
{
  WSACleanup();
}

/* ------------------------------------------------------------------ */
/* Windows Console helpers                                             */
/* ------------------------------------------------------------------ */

/*
 * Enable VT-100/ANSI processing on Windows Terminal (and modern
 * conhost).  Must be called before writing VT sequences to stdout.
 * Returns true on success.
 */
inline bool windows_enable_vt_mode( void )
{
  HANDLE h_out = GetStdHandle( STD_OUTPUT_HANDLE );
  if ( h_out == INVALID_HANDLE_VALUE ) {
    return false;
  }
  DWORD mode = 0;
  if ( !GetConsoleMode( h_out, &mode ) ) {
    return false;
  }
  mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  mode |= DISABLE_NEWLINE_AUTO_RETURN;
  return SetConsoleMode( h_out, mode ) != 0;
}

/*
 * Query current console window dimensions (columns × rows).
 * Returns true and sets *cols/*rows on success.
 */
inline bool windows_get_console_size( short* rows, short* cols )
{
  HANDLE h_out = GetStdHandle( STD_OUTPUT_HANDLE );
  if ( h_out == INVALID_HANDLE_VALUE ) {
    return false;
  }
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if ( !GetConsoleScreenBufferInfo( h_out, &csbi ) ) {
    return false;
  }
  *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  return true;
}

/*
 * Put stdin/stdout in binary mode so that VT sequences pass through
 * unmodified (no CRLF translation, etc.).
 */
inline void windows_set_binary_stdio( void )
{
  _setmode( 0 /* stdin */ , _O_BINARY );
  _setmode( 1 /* stdout */, _O_BINARY );
  _setmode( 2 /* stderr */, _O_BINARY );
}

/*
 * Set the console code page to UTF-8 (65001) so that multi-byte
 * characters sent/received as UTF-8 are handled correctly.
 */
inline void windows_set_utf8_codepage( void )
{
  SetConsoleCP( 65001 );
  SetConsoleOutputCP( 65001 );
}

#endif /* _WIN32 */

#endif /* WINDOWS_COMPAT_HPP */
