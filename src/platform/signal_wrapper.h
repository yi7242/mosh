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

#ifndef SIGNAL_WRAPPER_H
#define SIGNAL_WRAPPER_H

/* Platform-agnostic signal/event interface */
namespace Platform {

/* Platform-independent signal types */
enum SignalType {
  SIGNAL_WINCH,    /* Window size change */
  SIGNAL_TERM,     /* Termination request */
  SIGNAL_INT,      /* Interrupt (Ctrl+C) */
  SIGNAL_HUP,      /* Hangup */
  SIGNAL_PIPE,     /* Broken pipe */
  SIGNAL_CONT,     /* Continue */
  SIGNAL_CHLD,     /* Child process state change */
  SIGNAL_USR1,     /* User-defined signal 1 */
  SIGNAL_ALRM,     /* Alarm */
};

class SignalHandler {
public:
  virtual ~SignalHandler() {}

  /* Add a signal to be monitored */
  virtual void add_signal(SignalType sig) = 0;

  /* Remove a signal from monitoring */
  virtual void remove_signal(SignalType sig) = 0;

  /* Check if a signal has occurred since last check */
  virtual bool check_signal(SignalType sig) = 0;

  /* Clear the signal flag */
  virtual void clear_signal(SignalType sig) = 0;

  /* Get platform-specific handle for select/wait operations */
  virtual void* get_wait_handle() = 0;

  /* Create platform-specific signal handler */
  static SignalHandler* create();
};

} /* namespace Platform */

#endif /* SIGNAL_WRAPPER_H */
