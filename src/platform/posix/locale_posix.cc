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

#ifndef _WIN32

#include "locale_wrapper.h"
#include "locale_utils.h"
#include <locale.h>
#include <string.h>

namespace Platform {

/* POSIX locale implementation */
class PosixLocale : public Locale {
public:
  PosixLocale() {}
  virtual ~PosixLocale() {}

  /* Check if current locale is UTF-8 */
  virtual bool is_utf8() {
    return is_utf8_locale();
  }

  /* Set UTF-8 locale */
  virtual int set_utf8() {
    /* Try to set UTF-8 locale */
    if (setlocale(LC_ALL, "en_US.UTF-8") != NULL) {
      return 0;
    }
    if (setlocale(LC_ALL, "C.UTF-8") != NULL) {
      return 0;
    }
    /* Try generic UTF-8 locale */
    if (setlocale(LC_ALL, ".UTF-8") != NULL) {
      return 0;
    }
    return -1;
  }

  /* Get charset name */
  virtual const char* get_charset() {
    return locale_charset();
  }
};

/* Factory method */
Locale* Locale::create() {
  return new PosixLocale();
}

} /* namespace Platform */

#endif /* !_WIN32 */
