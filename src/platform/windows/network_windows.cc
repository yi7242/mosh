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

#include "network_wrapper.h"
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

namespace Platform {

/* Windows network implementation using Winsock */
class WindowsNetwork : public Network {
private:
  bool initialized;
  WSADATA wsaData;

public:
  WindowsNetwork() : initialized(false) {}
  virtual ~WindowsNetwork() {
    cleanup();
  }

  /* Initialize Winsock */
  virtual int initialize() {
    if (initialized) {
      return 0;
    }

    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
      return -1;
    }

    initialized = true;
    return 0;
  }

  /* Cleanup Winsock */
  virtual int cleanup() {
    if (!initialized) {
      return 0;
    }

    WSACleanup();
    initialized = false;
    return 0;
  }
};

/* Factory method */
Network* Network::create() {
  return new WindowsNetwork();
}

} /* namespace Platform */

#endif /* _WIN32 */
