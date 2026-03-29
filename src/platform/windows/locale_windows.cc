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

#ifdef _WIN32

#include "locale_wrapper.h"
#include <windows.h>

namespace Platform {

/* Windows locale implementation using code pages */
class WindowsLocale : public Locale {
private:
  UINT original_cp;
  UINT original_output_cp;

public:
  WindowsLocale() {
    original_cp = GetConsoleCP();
    original_output_cp = GetConsoleOutputCP();
  }

  virtual ~WindowsLocale() {}

  /* Check if current code page is UTF-8 */
  virtual bool is_utf8() {
    return GetConsoleCP() == CP_UTF8 && GetConsoleOutputCP() == CP_UTF8;
  }

  /* Set UTF-8 code page */
  virtual int set_utf8() {
    if (!SetConsoleCP(CP_UTF8)) {
      return -1;
    }
    if (!SetConsoleOutputCP(CP_UTF8)) {
      SetConsoleCP(original_cp);  /* Restore on failure */
      return -1;
    }
    return 0;
  }

  /* Get charset name */
  virtual const char* get_charset() {
    if (is_utf8()) {
      return "UTF-8";
    }

    UINT cp = GetConsoleOutputCP();
    switch (cp) {
    case 1252:
      return "CP1252"; /* Windows Western European */
    case 437:
      return "CP437";  /* OEM United States */
    case 850:
      return "CP850";  /* OEM Multilingual Latin 1 */
    default:
      return "UNKNOWN";
    }
  }
};

/* Factory method */
Locale* Locale::create() {
  return new WindowsLocale();
}

} /* namespace Platform */

#endif /* _WIN32 */
