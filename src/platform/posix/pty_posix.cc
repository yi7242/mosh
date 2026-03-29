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

#include "pty_wrapper.h"
#include "pty_compat.h"
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace Platform {

/* POSIX PTY implementation using existing pty_compat functions */
class PosixPty : public Pty {
public:
  PosixPty() {}
  virtual ~PosixPty() {}

  /* Fork a new process with a PTY */
  virtual pid_t fork_pty(int *master_fd, const PtySize *size) {
    struct winsize ws;

    if (size) {
      ws.ws_row = size->rows;
      ws.ws_col = size->cols;
      ws.ws_xpixel = size->xpixel;
      ws.ws_ypixel = size->ypixel;
    } else {
      ws.ws_row = 24;
      ws.ws_col = 80;
      ws.ws_xpixel = 0;
      ws.ws_ypixel = 0;
    }

    pid_t pid = my_forkpty(master_fd, NULL, NULL, &ws);
    return pid;
  }

  /* Resize the PTY */
  virtual int resize(int fd, const PtySize *size) {
    struct winsize ws;
    ws.ws_row = size->rows;
    ws.ws_col = size->cols;
    ws.ws_xpixel = size->xpixel;
    ws.ws_ypixel = size->ypixel;

    return ioctl(fd, TIOCSWINSZ, &ws);
  }

  /* Get current PTY size */
  virtual int get_size(int fd, PtySize *size) {
    struct winsize ws;

    if (ioctl(fd, TIOCGWINSZ, &ws) < 0) {
      return -1;
    }

    size->rows = ws.ws_row;
    size->cols = ws.ws_col;
    size->xpixel = ws.ws_xpixel;
    size->ypixel = ws.ws_ypixel;

    return 0;
  }
};

/* Factory method */
Pty* Pty::create() {
  return new PosixPty();
}

} /* namespace Platform */

#endif /* !_WIN32 */
