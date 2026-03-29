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

#include "terminal_wrapper.h"
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <map>

namespace Platform {

/* POSIX terminal I/O implementation using termios */
class PosixTerminal : public Terminal {
private:
  std::map<int, struct termios> saved_states;

public:
  PosixTerminal() {}
  virtual ~PosixTerminal() {}

  /* Save current terminal state */
  virtual int save_state(int fd) {
    struct termios state;

    if (tcgetattr(fd, &state) < 0) {
      return -1;
    }

    saved_states[fd] = state;
    return 0;
  }

  /* Restore saved terminal state */
  virtual int restore_state(int fd) {
    auto it = saved_states.find(fd);
    if (it == saved_states.end()) {
      return -1;
    }

    return tcsetattr(fd, TCSANOW, &it->second);
  }

  /* Set terminal mode (raw or normal) */
  virtual int set_mode(int fd, TerminalMode mode) {
    struct termios state;

    if (tcgetattr(fd, &state) < 0) {
      return -1;
    }

    if (mode == MODE_RAW) {
      cfmakeraw(&state);
    } else {
      /* Restore to saved state or use sane defaults */
      auto it = saved_states.find(fd);
      if (it != saved_states.end()) {
        state = it->second;
      }
    }

    return tcsetattr(fd, TCSANOW, &state);
  }

  /* Set window size */
  virtual int set_window_size(int fd, unsigned short rows, unsigned short cols) {
    struct winsize ws;
    ws.ws_row = rows;
    ws.ws_col = cols;
    ws.ws_xpixel = 0;
    ws.ws_ypixel = 0;

    return ioctl(fd, TIOCSWINSZ, &ws);
  }

  /* Get window size */
  virtual int get_window_size(int fd, unsigned short *rows, unsigned short *cols) {
    struct winsize ws;

    if (ioctl(fd, TIOCGWINSZ, &ws) < 0) {
      return -1;
    }

    *rows = ws.ws_row;
    *cols = ws.ws_col;

    return 0;
  }
};

/* Factory method */
Terminal* Terminal::create() {
  return new PosixTerminal();
}

} /* namespace Platform */

#endif /* !_WIN32 */
