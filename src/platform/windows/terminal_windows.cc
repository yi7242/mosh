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

#include "terminal_wrapper.h"
#include <windows.h>
#include <io.h>
#include <map>

namespace Platform {

/* Windows terminal I/O implementation using Console API */
class WindowsTerminal : public Terminal {
private:
  struct TerminalState {
    DWORD input_mode;
    DWORD output_mode;
    UINT input_cp;
    UINT output_cp;
  };

  std::map<int, TerminalState> saved_states;

  /* Get console handle from file descriptor */
  HANDLE get_console_handle(int fd) {
    if (fd == 0) {  /* stdin */
      return GetStdHandle(STD_INPUT_HANDLE);
    } else if (fd == 1) {  /* stdout */
      return GetStdHandle(STD_OUTPUT_HANDLE);
    } else if (fd == 2) {  /* stderr */
      return GetStdHandle(STD_ERROR_HANDLE);
    }
    return INVALID_HANDLE_VALUE;
  }

public:
  WindowsTerminal() {}
  virtual ~WindowsTerminal() {}

  /* Save current terminal state */
  virtual int save_state(int fd) {
    HANDLE hConsole = get_console_handle(fd);
    if (hConsole == INVALID_HANDLE_VALUE) {
      return -1;
    }

    TerminalState state;

    if (fd == 0) {  /* stdin */
      if (!GetConsoleMode(hConsole, &state.input_mode)) {
        return -1;
      }
      state.input_cp = GetConsoleCP();
    } else {  /* stdout/stderr */
      if (!GetConsoleMode(hConsole, &state.output_mode)) {
        return -1;
      }
      state.output_cp = GetConsoleOutputCP();
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

    HANDLE hConsole = get_console_handle(fd);
    if (hConsole == INVALID_HANDLE_VALUE) {
      return -1;
    }

    TerminalState& state = it->second;

    if (fd == 0) {  /* stdin */
      if (!SetConsoleMode(hConsole, state.input_mode)) {
        return -1;
      }
      SetConsoleCP(state.input_cp);
    } else {  /* stdout/stderr */
      if (!SetConsoleMode(hConsole, state.output_mode)) {
        return -1;
      }
      SetConsoleOutputCP(state.output_cp);
    }

    return 0;
  }

  /* Set terminal mode (raw or normal) */
  virtual int set_mode(int fd, TerminalMode mode) {
    HANDLE hConsole = get_console_handle(fd);
    if (hConsole == INVALID_HANDLE_VALUE) {
      return -1;
    }

    DWORD console_mode;
    if (!GetConsoleMode(hConsole, &console_mode)) {
      return -1;
    }

    if (mode == MODE_RAW) {
      if (fd == 0) {  /* stdin */
        /* Disable line input, echo, and processed input */
        console_mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
                         ENABLE_PROCESSED_INPUT);
        /* Enable virtual terminal input for escape sequences */
        console_mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
      } else {  /* stdout/stderr */
        /* Enable virtual terminal processing for ANSI sequences */
        console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        console_mode |= DISABLE_NEWLINE_AUTO_RETURN;
      }
    } else {  /* MODE_NORMAL */
      if (fd == 0) {  /* stdin */
        /* Enable line input, echo, and processed input */
        console_mode |= (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT |
                        ENABLE_PROCESSED_INPUT);
      } else {  /* stdout/stderr */
        /* Use default console mode */
        console_mode &= ~DISABLE_NEWLINE_AUTO_RETURN;
      }
    }

    if (!SetConsoleMode(hConsole, console_mode)) {
      return -1;
    }

    /* Set UTF-8 code page for proper character handling */
    if (mode == MODE_RAW) {
      if (fd == 0) {
        SetConsoleCP(CP_UTF8);
      } else {
        SetConsoleOutputCP(CP_UTF8);
      }
    }

    return 0;
  }

  /* Set window size */
  virtual int set_window_size(int fd, unsigned short rows, unsigned short cols) {
    HANDLE hConsole = get_console_handle(fd);
    if (hConsole == INVALID_HANDLE_VALUE) {
      return -1;
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
      return -1;
    }

    /* Resize the console screen buffer */
    COORD bufferSize;
    bufferSize.X = cols;
    bufferSize.Y = rows;

    /* Set buffer size first */
    if (!SetConsoleScreenBufferSize(hConsole, bufferSize)) {
      return -1;
    }

    /* Then set window size */
    SMALL_RECT windowSize;
    windowSize.Left = 0;
    windowSize.Top = 0;
    windowSize.Right = cols - 1;
    windowSize.Bottom = rows - 1;

    if (!SetConsoleWindowInfo(hConsole, TRUE, &windowSize)) {
      return -1;
    }

    return 0;
  }

  /* Get window size */
  virtual int get_window_size(int fd, unsigned short *rows, unsigned short *cols) {
    HANDLE hConsole = get_console_handle(fd);
    if (hConsole == INVALID_HANDLE_VALUE) {
      return -1;
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) {
      return -1;
    }

    *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    return 0;
  }
};

/* Factory method */
Terminal* Terminal::create() {
  return new WindowsTerminal();
}

} /* namespace Platform */

#endif /* _WIN32 */
