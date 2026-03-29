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

#include "signal_wrapper.h"
#include <windows.h>
#include <map>

namespace Platform {

/* Windows signal/event handler using Windows events */
class WindowsSignalHandler : public SignalHandler {
private:
  std::map<SignalType, bool> signal_flags;
  std::map<SignalType, HANDLE> signal_events;
  HANDLE event_handle;

  /* Console control handler for Ctrl+C, Ctrl+Break, etc. */
  static WindowsSignalHandler* instance;
  static BOOL WINAPI console_ctrl_handler(DWORD dwCtrlType) {
    if (!instance) return FALSE;

    switch (dwCtrlType) {
    case CTRL_C_EVENT:
      instance->signal_flags[SIGNAL_INT] = true;
      SetEvent(instance->event_handle);
      return TRUE;

    case CTRL_BREAK_EVENT:
      instance->signal_flags[SIGNAL_TERM] = true;
      SetEvent(instance->event_handle);
      return TRUE;

    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
      instance->signal_flags[SIGNAL_HUP] = true;
      SetEvent(instance->event_handle);
      return TRUE;

    default:
      return FALSE;
    }
  }

  /* Thread to monitor window size changes */
  static DWORD WINAPI window_resize_thread(LPVOID param) {
    WindowsSignalHandler* handler = static_cast<WindowsSignalHandler*>(param);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD lastSize = {0, 0};

    /* Get initial size */
    if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
      lastSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
      lastSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }

    while (true) {
      Sleep(500); /* Check every 500ms */

      if (GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        COORD newSize;
        newSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        newSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

        if (newSize.X != lastSize.X || newSize.Y != lastSize.Y) {
          handler->signal_flags[SIGNAL_WINCH] = true;
          SetEvent(handler->event_handle);
          lastSize = newSize;
        }
      }
    }

    return 0;
  }

  HANDLE resize_thread_handle;

public:
  WindowsSignalHandler() : resize_thread_handle(NULL) {
    /* Create event for signaling */
    event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);

    /* Set up console control handler */
    instance = this;
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
  }

  virtual ~WindowsSignalHandler() {
    SetConsoleCtrlHandler(console_ctrl_handler, FALSE);

    if (resize_thread_handle) {
      TerminateThread(resize_thread_handle, 0);
      CloseHandle(resize_thread_handle);
    }

    if (event_handle) {
      CloseHandle(event_handle);
    }

    for (auto& pair : signal_events) {
      CloseHandle(pair.second);
    }

    instance = NULL;
  }

  /* Add a signal to be monitored */
  virtual void add_signal(SignalType sig) {
    signal_flags[sig] = false;

    /* Start window resize monitoring thread if SIGNAL_WINCH is added */
    if (sig == SIGNAL_WINCH && !resize_thread_handle) {
      resize_thread_handle = CreateThread(
        NULL, 0, window_resize_thread, this, 0, NULL);
    }
  }

  /* Remove a signal from monitoring */
  virtual void remove_signal(SignalType sig) {
    signal_flags.erase(sig);

    /* Stop window resize thread if SIGNAL_WINCH is removed */
    if (sig == SIGNAL_WINCH && resize_thread_handle) {
      TerminateThread(resize_thread_handle, 0);
      CloseHandle(resize_thread_handle);
      resize_thread_handle = NULL;
    }
  }

  /* Check if a signal has occurred since last check */
  virtual bool check_signal(SignalType sig) {
    auto it = signal_flags.find(sig);
    return it != signal_flags.end() && it->second;
  }

  /* Clear the signal flag */
  virtual void clear_signal(SignalType sig) {
    auto it = signal_flags.find(sig);
    if (it != signal_flags.end()) {
      it->second = false;
    }
  }

  /* Get platform-specific handle for select/wait operations */
  virtual void* get_wait_handle() {
    return event_handle;
  }
};

/* Static member initialization */
WindowsSignalHandler* WindowsSignalHandler::instance = NULL;

/* Factory method */
SignalHandler* SignalHandler::create() {
  return new WindowsSignalHandler();
}

} /* namespace Platform */

#endif /* _WIN32 */
