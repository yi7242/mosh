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

#include "src/include/config.h"

#include <cerrno>
#include <clocale>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#ifdef _WIN32
#include "src/include/windows_compat.h"
#else
#if HAVE_LANGINFO_H
#include <langinfo.h>
#endif
#endif /* _WIN32 */

#include "src/util/locale_utils.h"

const std::string LocaleVar::str( void ) const
{
  if ( name.empty() ) {
    return std::string( "[no charset variables]" );
  }
  return name + "=" + value;
}

const LocaleVar get_ctype( void )
{
  /* Reimplement the search logic, just for diagnostics */
  if ( const char* all = getenv( "LC_ALL" ) ) {
    return LocaleVar( "LC_ALL", all );
  } else if ( const char* ctype = getenv( "LC_CTYPE" ) ) {
    return LocaleVar( "LC_CTYPE", ctype );
  } else if ( const char* lang = getenv( "LANG" ) ) {
    return LocaleVar( "LANG", lang );
  }
  return LocaleVar( "", "" );
}

const char* locale_charset( void )
{
#ifdef _WIN32
  /* On Windows, check the active console output code page.
     We enforce UTF-8 (code page 65001) at startup via
     windows_set_utf8_codepage(), so this should always return UTF-8. */
  static const char utf8_name[] = "UTF-8";
  static const char ascii_name[] = "US-ASCII";

  UINT cp = GetConsoleOutputCP();
  if ( cp == 65001 ) {
    return utf8_name;
  }
  /* Code page 20127 is US-ASCII */
  if ( cp == 20127 ) {
    return ascii_name;
  }
  static char cp_name[16];
  snprintf( cp_name, sizeof cp_name, "CP%u", cp );
  return cp_name;
#else
  static const char ASCII_name[] = "US-ASCII";

  /* Produce more pleasant name of US-ASCII */
  const char* ret = nl_langinfo( CODESET );

  if ( strcmp( ret, "ANSI_X3.4-1968" ) == 0 ) {
    ret = ASCII_name;
  }

  return ret;
#endif /* _WIN32 */
}

bool is_utf8_locale( void )
{
  /* Verify locale calls for UTF-8 */
  if ( strcmp( locale_charset(), "UTF-8" ) != 0 && strcmp( locale_charset(), "utf-8" ) != 0 ) {
    return false;
  }
  return true;
}

void set_native_locale( void )
{
#ifdef _WIN32
  /* On Windows, set both console code pages to UTF-8 so that the
     narrow-character locale_charset() check passes.  Also call
     setlocale() so that C library string functions work with UTF-8. */
  windows_set_utf8_codepage();
  setlocale( LC_ALL, ".UTF-8" );
#else
  /* Adopt native locale */
  if ( NULL == setlocale( LC_ALL, "" ) ) {
    int saved_errno = errno;
    if ( saved_errno == ENOENT ) {
      LocaleVar ctype( get_ctype() );
      fprintf( stderr, "The locale requested by %s isn't available here.\n", ctype.str().c_str() );
      if ( !ctype.name.empty() ) {
        fprintf( stderr, "Running `locale-gen %s' may be necessary.\n\n", ctype.value.c_str() );
      }
    } else {
      errno = saved_errno;
      perror( "setlocale" );
    }
  }
#endif /* _WIN32 */
}

void clear_locale_variables( void )
{
#ifndef _WIN32
  unsetenv( "LANG" );
  unsetenv( "LANGUAGE" );
  unsetenv( "LC_CTYPE" );
  unsetenv( "LC_NUMERIC" );
  unsetenv( "LC_TIME" );
  unsetenv( "LC_COLLATE" );
  unsetenv( "LC_MONETARY" );
  unsetenv( "LC_MESSAGES" );
  unsetenv( "LC_PAPER" );
  unsetenv( "LC_NAME" );
  unsetenv( "LC_ADDRESS" );
  unsetenv( "LC_TELEPHONE" );
  unsetenv( "LC_MEASUREMENT" );
  unsetenv( "LC_IDENTIFICATION" );
  unsetenv( "LC_ALL" );
#else
  /* On Windows, environment variable manipulation happens differently.
     The server (which calls this function) does not run on Windows;
     provide a no-op stub. */
#endif /* _WIN32 */
}
