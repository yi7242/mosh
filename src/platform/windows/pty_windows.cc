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

#include "pty_wrapper.h"
#include <windows.h>
#include <process.h>
#include <io.h>
#include <fcntl.h>
#include <map>
#include <string>

/* ConPTY API functions (Windows 10 1809+) */
#ifndef PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE
#define PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE \
  ProcThreadAttributeValue(22, FALSE, TRUE, FALSE)

typedef VOID* HPCON;

extern "C" {
HRESULT WINAPI CreatePseudoConsole(COORD size, HANDLE hInput, HANDLE hOutput,
                                    DWORD dwFlags, HPCON* phPC);
HRESULT WINAPI ResizePseudoConsole(HPCON hPC, COORD size);
VOID WINAPI ClosePseudoConsole(HPCON hPC);
}
#endif

namespace Platform {

/* Windows PTY implementation using ConPTY */
class WindowsPty : public Pty {
private:
  struct PtyInfo {
    HPCON hPC;
    HANDLE hInput;
    HANDLE hOutput;
    HANDLE hProcess;
    DWORD dwProcessId;
  };

  static std::map<int, PtyInfo> pty_map;
  static int next_fd;

  /* Create pipes for ConPTY */
  bool create_pipes(HANDLE* hInputRead, HANDLE* hInputWrite,
                   HANDLE* hOutputRead, HANDLE* hOutputWrite) {
    SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

    if (!CreatePipe(hInputRead, hInputWrite, &sa, 0)) {
      return false;
    }
    if (!SetHandleInformation(*hInputWrite, HANDLE_FLAG_INHERIT, 0)) {
      CloseHandle(*hInputRead);
      CloseHandle(*hInputWrite);
      return false;
    }

    if (!CreatePipe(hOutputRead, hOutputWrite, &sa, 0)) {
      CloseHandle(*hInputRead);
      CloseHandle(*hInputWrite);
      return false;
    }
    if (!SetHandleInformation(*hOutputRead, HANDLE_FLAG_INHERIT, 0)) {
      CloseHandle(*hInputRead);
      CloseHandle(*hInputWrite);
      CloseHandle(*hOutputRead);
      CloseHandle(*hOutputWrite);
      return false;
    }

    return true;
  }

public:
  WindowsPty() {}
  virtual ~WindowsPty() {}

  /* Fork a new process with a PTY using ConPTY */
  virtual pid_t fork_pty(int *master_fd, const PtySize *size) {
    HANDLE hInputRead, hInputWrite, hOutputRead, hOutputWrite;

    /* Create pipes for ConPTY I/O */
    if (!create_pipes(&hInputRead, &hInputWrite, &hOutputRead, &hOutputWrite)) {
      return -1;
    }

    /* Create ConPTY */
    COORD ptySize = {
      static_cast<SHORT>(size ? size->cols : 80),
      static_cast<SHORT>(size ? size->rows : 24)
    };

    HPCON hPC;
    HRESULT hr = CreatePseudoConsole(ptySize, hInputRead, hOutputWrite, 0, &hPC);
    CloseHandle(hInputRead);
    CloseHandle(hOutputWrite);

    if (FAILED(hr)) {
      CloseHandle(hInputWrite);
      CloseHandle(hOutputRead);
      return -1;
    }

    /* Prepare startup info with ConPTY */
    SIZE_T attrListSize = 0;
    InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);
    LPPROC_THREAD_ATTRIBUTE_LIST attrList =
        (LPPROC_THREAD_ATTRIBUTE_LIST)malloc(attrListSize);

    if (!InitializeProcThreadAttributeList(attrList, 1, 0, &attrListSize)) {
      free(attrList);
      ClosePseudoConsole(hPC);
      CloseHandle(hInputWrite);
      CloseHandle(hOutputRead);
      return -1;
    }

    if (!UpdateProcThreadAttribute(attrList, 0,
                                   PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                                   hPC, sizeof(HPCON), NULL, NULL)) {
      DeleteProcThreadAttributeList(attrList);
      free(attrList);
      ClosePseudoConsole(hPC);
      CloseHandle(hInputWrite);
      CloseHandle(hOutputRead);
      return -1;
    }

    /* Create process with ConPTY */
    STARTUPINFOEX siEx = {0};
    siEx.StartupInfo.cb = sizeof(STARTUPINFOEX);
    siEx.lpAttributeList = attrList;

    PROCESS_INFORMATION pi = {0};

    /* Get shell from environment or use default */
    const char* shell = getenv("SHELL");
    if (!shell || !*shell) {
      shell = getenv("COMSPEC");
      if (!shell || !*shell) {
        shell = "cmd.exe";
      }
    }

    std::string cmdLine = shell;
    wchar_t* wCmdLine = new wchar_t[cmdLine.length() + 1];
    MultiByteToWideChar(CP_UTF8, 0, cmdLine.c_str(), -1, wCmdLine,
                        static_cast<int>(cmdLine.length() + 1));

    BOOL success = CreateProcessW(
      NULL,                      /* Application name */
      wCmdLine,                  /* Command line */
      NULL,                      /* Process security attributes */
      NULL,                      /* Thread security attributes */
      FALSE,                     /* Inherit handles */
      EXTENDED_STARTUPINFO_PRESENT, /* Creation flags */
      NULL,                      /* Environment */
      NULL,                      /* Current directory */
      &siEx.StartupInfo,         /* Startup info */
      &pi                        /* Process information */
    );

    delete[] wCmdLine;
    DeleteProcThreadAttributeList(attrList);
    free(attrList);

    if (!success) {
      ClosePseudoConsole(hPC);
      CloseHandle(hInputWrite);
      CloseHandle(hOutputRead);
      return -1;
    }

    /* Create a pseudo file descriptor */
    int fd = next_fd++;

    /* Store PTY info */
    PtyInfo info;
    info.hPC = hPC;
    info.hInput = hInputWrite;
    info.hOutput = hOutputRead;
    info.hProcess = pi.hProcess;
    info.dwProcessId = pi.dwProcessId;
    pty_map[fd] = info;

    CloseHandle(pi.hThread);

    *master_fd = fd;
    return static_cast<pid_t>(pi.dwProcessId);
  }

  /* Resize the PTY */
  virtual int resize(int fd, const PtySize *size) {
    auto it = pty_map.find(fd);
    if (it == pty_map.end()) {
      return -1;
    }

    COORD ptySize = {
      static_cast<SHORT>(size->cols),
      static_cast<SHORT>(size->rows)
    };

    HRESULT hr = ResizePseudoConsole(it->second.hPC, ptySize);
    return SUCCEEDED(hr) ? 0 : -1;
  }

  /* Get current PTY size */
  virtual int get_size(int fd, PtySize *size) {
    /* ConPTY doesn't provide a direct way to query size,
       so we track it separately or return the console size */
    auto it = pty_map.find(fd);
    if (it == pty_map.end()) {
      return -1;
    }

    /* For now, return default size - this would need to be tracked */
    size->rows = 24;
    size->cols = 80;
    size->xpixel = 0;
    size->ypixel = 0;
    return 0;
  }
};

/* Static member initialization */
std::map<int, WindowsPty::PtyInfo> WindowsPty::pty_map;
int WindowsPty::next_fd = 1000; /* Start at high number to avoid conflicts */

/* Factory method */
Pty* Pty::create() {
  return new WindowsPty();
}

} /* namespace Platform */

#endif /* _WIN32 */
