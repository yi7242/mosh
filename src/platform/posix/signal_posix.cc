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

#include "signal_wrapper.h"
#include <signal.h>
#include <map>

namespace Platform {

/* POSIX signal handler wrapper */
class PosixSignalHandler : public SignalHandler {
private:
  std::map<SignalType, int> signal_map;
  std::map<SignalType, bool> signal_flags;
  static PosixSignalHandler* instance;

  /* Convert platform-independent signal to POSIX signal */
  int to_posix_signal(SignalType sig) {
    switch (sig) {
    case SIGNAL_WINCH: return SIGWINCH;
    case SIGNAL_TERM:  return SIGTERM;
    case SIGNAL_INT:   return SIGINT;
    case SIGNAL_HUP:   return SIGHUP;
    case SIGNAL_PIPE:  return SIGPIPE;
    case SIGNAL_CONT:  return SIGCONT;
    case SIGNAL_CHLD:  return SIGCHLD;
    case SIGNAL_USR1:  return SIGUSR1;
    case SIGNAL_ALRM:  return SIGALRM;
    default: return -1;
    }
  }

  /* Convert POSIX signal to platform-independent signal */
  SignalType from_posix_signal(int sig) {
    switch (sig) {
    case SIGWINCH: return SIGNAL_WINCH;
    case SIGTERM:  return SIGNAL_TERM;
    case SIGINT:   return SIGNAL_INT;
    case SIGHUP:   return SIGNAL_HUP;
    case SIGPIPE:  return SIGNAL_PIPE;
    case SIGCONT:  return SIGNAL_CONT;
    case SIGCHLD:  return SIGNAL_CHLD;
    case SIGUSR1:  return SIGNAL_USR1;
    case SIGALRM:  return SIGNAL_ALRM;
    default:       return SIGNAL_WINCH; /* Default fallback */
    }
  }

  /* Signal handler function */
  static void signal_handler(int sig) {
    if (instance) {
      SignalType type = instance->from_posix_signal(sig);
      instance->signal_flags[type] = true;
    }
  }

public:
  PosixSignalHandler() {
    instance = this;
  }

  virtual ~PosixSignalHandler() {
    /* Restore default signal handlers */
    for (auto& pair : signal_map) {
      signal(pair.second, SIG_DFL);
    }
    instance = NULL;
  }

  /* Add a signal to be monitored */
  virtual void add_signal(SignalType sig) {
    int posix_sig = to_posix_signal(sig);
    if (posix_sig >= 0) {
      signal_map[sig] = posix_sig;
      signal_flags[sig] = false;
      signal(posix_sig, signal_handler);
    }
  }

  /* Remove a signal from monitoring */
  virtual void remove_signal(SignalType sig) {
    auto it = signal_map.find(sig);
    if (it != signal_map.end()) {
      signal(it->second, SIG_DFL);
      signal_map.erase(it);
      signal_flags.erase(sig);
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
    /* POSIX doesn't use handles for signals, they're delivered asynchronously */
    return NULL;
  }
};

/* Static member initialization */
PosixSignalHandler* PosixSignalHandler::instance = NULL;

/* Factory method */
SignalHandler* SignalHandler::create() {
  return new PosixSignalHandler();
}

} /* namespace Platform */

#endif /* !_WIN32 */
