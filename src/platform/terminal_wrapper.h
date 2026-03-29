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

#ifndef TERMINAL_WRAPPER_H
#define TERMINAL_WRAPPER_H

#include <sys/types.h>

/* Platform-agnostic terminal I/O interface */
namespace Platform {

/* Terminal mode flags */
enum TerminalMode {
  MODE_NORMAL,    /* Normal cooked mode */
  MODE_RAW,       /* Raw mode (no echo, no processing) */
};

class Terminal {
public:
  virtual ~Terminal() {}

  /* Save current terminal state */
  virtual int save_state(int fd) = 0;

  /* Restore saved terminal state */
  virtual int restore_state(int fd) = 0;

  /* Set terminal mode (raw or normal) */
  virtual int set_mode(int fd, TerminalMode mode) = 0;

  /* Set window size */
  virtual int set_window_size(int fd, unsigned short rows, unsigned short cols) = 0;

  /* Get window size */
  virtual int get_window_size(int fd, unsigned short *rows, unsigned short *cols) = 0;

  /* Create platform-specific terminal implementation */
  static Terminal* create();
};

} /* namespace Platform */

#endif /* TERMINAL_WRAPPER_H */
