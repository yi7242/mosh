AM_CPPFLAGS = -I$(srcdir)/.. -I$(builddir)/..
AM_CXXFLAGS = $(WARNING_CXXFLAGS) $(PICKY_CXXFLAGS) $(HARDEN_CFLAGS) $(MISC_CXXFLAGS)
AM_LDFLAGS = $(HARDEN_LDFLAGS)

noinst_LIBRARIES = libplatform.a

# Platform abstraction header files
platform_headers = \
	pty_wrapper.h \
	signal_wrapper.h \
	terminal_wrapper.h \
	network_wrapper.h \
	locale_wrapper.h

# Platform-specific source files
if WINDOWS
platform_sources = \
	windows/pty_windows.cc \
	windows/signal_windows.cc \
	windows/terminal_windows.cc \
	windows/network_windows.cc \
	windows/locale_windows.cc
else
platform_sources = \
	posix/pty_posix.cc \
	posix/signal_posix.cc \
	posix/terminal_posix.cc \
	posix/network_posix.cc \
	posix/locale_posix.cc
endif

libplatform_a_SOURCES = $(platform_headers) $(platform_sources)
