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

#include "src/include/config.h"

#ifdef _WIN32
#include "src/include/windows_compat.h"
#include <conio.h>  /* _getch() for "press any key to resume" on Windows */
#endif

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef _WIN32
#include <clocale>
#include <csignal>
#include <ctime>

#include <err.h>
#include <pwd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#if HAVE_PTY_H
#include <pty.h>
#elif HAVE_UTIL_H
#include <util.h>
#endif
#endif /* !_WIN32 */

#include "src/statesync/completeterminal.h"
#include "src/statesync/user.h"
#include "src/util/fatal_assert.h"
#include "src/util/locale_utils.h"
#ifndef _WIN32
#include "src/util/pty_compat.h"
#endif
#include "src/util/select.h"
#include "src/util/swrite.h"
#include "src/util/timestamp.h"
#include "stmclient.h"

#include "src/network/networktransport-impl.h"

void STMClient::resume( void )
{
#ifdef _WIN32
  /* Restore Windows console input mode */
  SetConsoleMode( h_stdin, saved_input_mode );
  /* Restore Windows console output mode (with VT enabled) */
  DWORD out_mode = saved_output_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
  SetConsoleMode( h_stdout, out_mode );
#else
  /* Restore termios state */
  if ( tcsetattr( STDIN_FILENO, TCSANOW, &raw_termios ) < 0 ) {
    perror( "tcsetattr" );
    exit( 1 );
  }
#endif

  /* Put terminal in application-cursor-key mode */
  swrite( STDOUT_FILENO, display.open().c_str() );

  /* Flag that outer terminal state is unknown */
  repaint_requested = true;
}

void STMClient::init( void )
{
#ifdef _WIN32
  /* Windows: set up console for UTF-8 and VT processing */
  windows_set_utf8_codepage();
  windows_set_binary_stdio();

  h_stdin = GetStdHandle( STD_INPUT_HANDLE );
  h_stdout = GetStdHandle( STD_OUTPUT_HANDLE );

  if ( h_stdin == INVALID_HANDLE_VALUE || h_stdout == INVALID_HANDLE_VALUE ) {
    fputs( "mosh-client: cannot get console handles\n", stderr );
    exit( 1 );
  }

  /* Save original console modes */
  if ( !GetConsoleMode( h_stdin, &saved_input_mode ) ) {
    fputs( "mosh-client: cannot get stdin console mode\n", stderr );
    exit( 1 );
  }
  if ( !GetConsoleMode( h_stdout, &saved_output_mode ) ) {
    fputs( "mosh-client: cannot get stdout console mode\n", stderr );
    exit( 1 );
  }

  /* Enable VT processing on output */
  DWORD out_mode = saved_output_mode
                   | ENABLE_VIRTUAL_TERMINAL_PROCESSING
                   | DISABLE_NEWLINE_AUTO_RETURN;
  if ( !SetConsoleMode( h_stdout, out_mode ) ) {
    fputs( "mosh-client: cannot enable VT processing on stdout.\n"
           "Ensure you are running in Windows Terminal or a console that\n"
           "supports ENABLE_VIRTUAL_TERMINAL_PROCESSING.\n",
           stderr );
    exit( 1 );
  }

  /* Put stdin in raw mode:
     - ENABLE_VIRTUAL_TERMINAL_INPUT: pass VT sequences through
     - Remove ENABLE_LINE_INPUT:       no line buffering
     - Remove ENABLE_ECHO_INPUT:       no echo
     - Keep ENABLE_PROCESSED_INPUT:    so Ctrl-C can still generate events
  */
  DWORD in_mode = ( saved_input_mode & ~( ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT ) )
                  | ENABLE_VIRTUAL_TERMINAL_INPUT;
  if ( !SetConsoleMode( h_stdin, in_mode ) ) {
    perror( "SetConsoleMode stdin" );
    exit( 1 );
  }
#else
  if ( !is_utf8_locale() ) {
    LocaleVar native_ctype = get_ctype();
    std::string native_charset( locale_charset() );

    fprintf( stderr,
             "mosh-client needs a UTF-8 native locale to run.\n\n"
             "Unfortunately, the client's environment (%s) specifies\n"
             "the character set \"%s\".\n\n",
             native_ctype.str().c_str(),
             native_charset.c_str() );
    int unused __attribute( ( unused ) ) = system( "locale" );
    exit( 1 );
  }

  /* Verify terminal configuration */
  if ( tcgetattr( STDIN_FILENO, &saved_termios ) < 0 ) {
    perror( "tcgetattr" );
    exit( 1 );
  }

  /* Put terminal driver in raw mode */
  raw_termios = saved_termios;

#ifdef HAVE_IUTF8
  if ( !( raw_termios.c_iflag & IUTF8 ) ) {
    //    fprintf( stderr, "Warning: Locale is UTF-8 but termios IUTF8 flag not set. Setting IUTF8 flag.\n" );
    /* Probably not really necessary since we are putting terminal driver into raw mode anyway. */
    raw_termios.c_iflag |= IUTF8;
  }
#endif /* HAVE_IUTF8 */

  cfmakeraw( &raw_termios );

  if ( tcsetattr( STDIN_FILENO, TCSANOW, &raw_termios ) < 0 ) {
    perror( "tcsetattr" );
    exit( 1 );
  }
#endif /* _WIN32 */

  /* Put terminal in application-cursor-key mode */
  swrite( STDOUT_FILENO, display.open().c_str() );

  /* Add our name to window title */
  if ( !getenv( "MOSH_TITLE_NOPREFIX" ) ) {
    overlays.set_title_prefix( std::wstring( L"[mosh] " ) );
  }

  /* Set terminal escape key. */
  const char* escape_key_env;
  if ( ( escape_key_env = getenv( "MOSH_ESCAPE_KEY" ) ) != NULL ) {
    if ( strlen( escape_key_env ) == 1 ) {
      escape_key = (int)escape_key_env[0];
      if ( escape_key > 0 && escape_key < 128 ) {
        if ( escape_key < 32 ) {
          /* If escape is ctrl-something, pass it with repeating the key without ctrl. */
          escape_pass_key = escape_key + (int)'@';
        } else {
          /* If escape is something else, pass it with repeating the key itself. */
          escape_pass_key = escape_key;
        }
        if ( escape_pass_key >= 'A' && escape_pass_key <= 'Z' ) {
          /* If escape pass is an upper case character, define optional version
             as lower case of the same. */
          escape_pass_key2 = escape_pass_key + (int)'a' - (int)'A';
        } else {
          escape_pass_key2 = escape_pass_key;
        }
      } else {
        escape_key = 0x1E;
        escape_pass_key = '^';
        escape_pass_key2 = '^';
      }
    } else if ( strlen( escape_key_env ) == 0 ) {
      escape_key = -1;
    } else {
      escape_key = 0x1E;
      escape_pass_key = '^';
      escape_pass_key2 = '^';
    }
  } else {
    escape_key = 0x1E;
    escape_pass_key = '^';
    escape_pass_key2 = '^';
  }

  /* There are so many better ways to shoot oneself into leg than
     setting escape key to Ctrl-C, Ctrl-D, NewLine, Ctrl-L or CarriageReturn
     that we just won't allow that. */
  if ( escape_key == 0x03 || escape_key == 0x04 || escape_key == 0x0A || escape_key == 0x0C
       || escape_key == 0x0D ) {
    escape_key = 0x1E;
    escape_pass_key = '^';
    escape_pass_key2 = '^';
  }

  /* Adjust escape help differently if escape is a control character. */
  if ( escape_key > 0 ) {
    char escape_pass_name_buf[16];
    char escape_key_name_buf[16];
    snprintf( escape_pass_name_buf, sizeof escape_pass_name_buf, "\"%c\"", escape_pass_key );
    if ( escape_key < 32 ) {
      snprintf( escape_key_name_buf, sizeof escape_key_name_buf, "Ctrl-%c", escape_pass_key );
      escape_requires_lf = false;
    } else {
      snprintf( escape_key_name_buf, sizeof escape_key_name_buf, "\"%c\"", escape_key );
      escape_requires_lf = true;
    }
    std::string tmp;
    tmp = std::string( escape_pass_name_buf );
    std::wstring escape_pass_name = std::wstring( tmp.begin(), tmp.end() );
    tmp = std::string( escape_key_name_buf );
    std::wstring escape_key_name = std::wstring( tmp.begin(), tmp.end() );
    escape_key_help
      = L"Commands: Ctrl-Z suspends, \".\" quits, " + escape_pass_name + L" gives literal " + escape_key_name;
    overlays.get_notification_engine().set_escape_key_string( tmp );
  }
  wchar_t tmp[128];
  swprintf( tmp, 128, L"Nothing received from server on UDP port %s.", port.c_str() );
  connecting_notification = std::wstring( tmp );
}

void STMClient::shutdown( void )
{
  /* Restore screen state */
  overlays.get_notification_engine().set_notification_string( std::wstring( L"" ) );
  overlays.get_notification_engine().server_heard( timestamp() );
  overlays.set_title_prefix( std::wstring( L"" ) );
  output_new_frame();

  /* Restore terminal and terminal-driver state */
  swrite( STDOUT_FILENO, display.close().c_str() );

#ifdef _WIN32
  if ( h_stdin != INVALID_HANDLE_VALUE ) {
    SetConsoleMode( h_stdin, saved_input_mode );
  }
  if ( h_stdout != INVALID_HANDLE_VALUE ) {
    SetConsoleMode( h_stdout, saved_output_mode );
  }
#else
  if ( tcsetattr( STDIN_FILENO, TCSANOW, &saved_termios ) < 0 ) {
    perror( "tcsetattr" );
    exit( 1 );
  }
#endif

  if ( still_connecting() ) {
    fprintf( stderr,
             "\nmosh did not make a successful connection to %s:%s.\n"
             "Please verify that UDP port %s is not firewalled and can reach the server.\n\n"
             "(By default, mosh uses a UDP port between 60000 and 61000. The -p option\n"
             "selects a specific UDP port number.)\n",
             ip.c_str(),
             port.c_str(),
             port.c_str() );
  } else if ( network && !clean_shutdown ) {
    fputs( "\n\nmosh did not shut down cleanly. Please note that the\n"
           "mosh-server process may still be running on the server.\n",
           stderr );
  }
}

void STMClient::main_init( void )
{
  Select& sel = Select::get_instance();
#ifdef _WIN32
  /* On Windows, signal handling is done via SetConsoleCtrlHandler in select.h.
     SIGWINCH is simulated by detecting console size changes in the main loop. */
  sel.add_signal( SIGTERM );
  sel.add_signal( SIGINT );
#else
  sel.add_signal( SIGWINCH );
  sel.add_signal( SIGTERM );
  sel.add_signal( SIGINT );
  sel.add_signal( SIGHUP );
  sel.add_signal( SIGPIPE );
  sel.add_signal( SIGCONT );
#endif

#ifdef _WIN32
  /* Get initial window size using Windows Console API */
  if ( !windows_get_console_size( &window_rows, &window_cols ) ) {
    fputs( "ioctl TIOCGWINSZ (GetConsoleScreenBufferInfo)\n", stderr );
    window_rows = 24;
    window_cols = 80;
  }
  /* local state */
  local_framebuffer = Terminal::Framebuffer( window_cols, window_rows );
#else
  /* get initial window size */
  if ( ioctl( STDIN_FILENO, TIOCGWINSZ, &window_size ) < 0 ) {
    perror( "ioctl TIOCGWINSZ" );
    return;
  }

  /* local state */
  local_framebuffer = Terminal::Framebuffer( window_size.ws_col, window_size.ws_row );
#endif

  new_state = Terminal::Framebuffer( 1, 1 );

  /* initialize screen */
  std::string init = display.new_frame( false, local_framebuffer, local_framebuffer );
  swrite( STDOUT_FILENO, init.data(), init.size() );

  /* open network */
  Network::UserStream blank;
#ifdef _WIN32
  Terminal::Complete local_terminal( window_cols, window_rows );
#else
  Terminal::Complete local_terminal( window_size.ws_col, window_size.ws_row );
#endif
  network = NetworkPointer( new NetworkType( blank, local_terminal, key.c_str(), ip.c_str(), port.c_str() ) );

  network->set_send_delay( 1 ); /* minimal delay on outgoing keystrokes */

  /* tell server the size of the terminal */
#ifdef _WIN32
  network->get_current_state().push_back( Parser::Resize( window_cols, window_rows ) );
#else
  network->get_current_state().push_back( Parser::Resize( window_size.ws_col, window_size.ws_row ) );
#endif

  /* be noisy as necessary */
  network->set_verbose( verbose );
  Select::set_verbose( verbose );
}

void STMClient::output_new_frame( void )
{
  if ( !network ) { /* clean shutdown even when not initialized */
    return;
  }

  /* fetch target state */
  new_state = network->get_latest_remote_state().state.get_fb();

  /* apply local overlays */
  overlays.apply( new_state );

  /* calculate minimal difference from where we are */
  const std::string diff( display.new_frame( !repaint_requested, local_framebuffer, new_state ) );
  swrite( STDOUT_FILENO, diff.data(), diff.size() );

  repaint_requested = false;

  local_framebuffer = new_state;
}

void STMClient::process_network_input( void )
{
  network->recv();

  /* Now give hints to the overlays */
  overlays.get_notification_engine().server_heard( network->get_latest_remote_state().timestamp );
  overlays.get_notification_engine().server_acked( network->get_sent_state_acked_timestamp() );

  overlays.get_prediction_engine().set_local_frame_acked( network->get_sent_state_acked() );
  overlays.get_prediction_engine().set_send_interval( network->send_interval() );
  overlays.get_prediction_engine().set_local_frame_late_acked(
    network->get_latest_remote_state().state.get_echo_ack() );
}

bool STMClient::process_user_input( int fd )
{
  const int buf_size = 16384;
  char buf[buf_size];

  /* fill buffer if possible */
#ifdef _WIN32
  /* On Windows, use _read() (from <io.h>) for file descriptor 0 (stdin).
     In VT input mode, bytes are passed through as-is, including UTF-8 and
     escape sequences. */
  int bytes_read = _read( fd, buf, buf_size );
#else
  ssize_t bytes_read = read( fd, buf, buf_size );
#endif
  if ( bytes_read == 0 ) { /* EOF */
    return false;
  } else if ( bytes_read < 0 ) {
    perror( "read" );
    return false;
  }

  NetworkType& net = *network;

  if ( net.shutdown_in_progress() ) {
    return true;
  }
  overlays.get_prediction_engine().set_local_frame_sent( net.get_sent_state_last() );

  /* Don't predict for bulk data. */
  bool paste = bytes_read > 100;
  if ( paste ) {
    overlays.get_prediction_engine().reset();
  }

  for ( int i = 0; i < bytes_read; i++ ) {
    char the_byte = buf[i];

    if ( !paste ) {
      overlays.get_prediction_engine().new_user_byte( the_byte, local_framebuffer );
    }

    if ( quit_sequence_started ) {
      if ( the_byte == '.' ) { /* Quit sequence is Ctrl-^ . */
        if ( net.has_remote_addr() && ( !net.shutdown_in_progress() ) ) {
          overlays.get_notification_engine().set_notification_string( std::wstring( L"Exiting on user request..." ),
                                                                      true );
          net.start_shutdown();
          return true;
        }
        return false;
      } else if ( the_byte == 0x1a ) { /* Suspend sequence is escape_key Ctrl-Z */
        /* Restore terminal and terminal-driver state */
        swrite( STDOUT_FILENO, display.close().c_str() );

#ifdef _WIN32
        /* Windows does not have SIGSTOP / job control.
           Just restore the console mode and print the "suspended" message.
           The user can manually bring the client back by pressing a key. */
        if ( h_stdin != INVALID_HANDLE_VALUE ) {
          SetConsoleMode( h_stdin, saved_input_mode );
        }
        if ( h_stdout != INVALID_HANDLE_VALUE ) {
          SetConsoleMode( h_stdout, saved_output_mode );
        }
        fputs( "\n\033[37;44m[mosh: suspend not supported on Windows. Press Enter to resume.]\033[m\n",
               stdout );
        fflush( NULL );
        /* Wait for any keypress, then re-enter raw mode */
        int dummy = _getch();
        (void)dummy;
        resume();
#else
        if ( tcsetattr( STDIN_FILENO, TCSANOW, &saved_termios ) < 0 ) {
          perror( "tcsetattr" );
          exit( 1 );
        }

        fputs( "\n\033[37;44m[mosh is suspended.]\033[m\n", stdout );

        fflush( NULL );

        /* actually suspend */
        kill( 0, SIGSTOP );

        resume();
#endif
      } else if ( ( the_byte == escape_pass_key ) || ( the_byte == escape_pass_key2 ) ) {
        /* Emulation sequence to type escape_key is escape_key +
           escape_pass_key (that is escape key without Ctrl) */
        net.get_current_state().push_back( Parser::UserByte( escape_key ) );
      } else {
        /* Escape key followed by anything other than . and ^ gets sent literally */
        net.get_current_state().push_back( Parser::UserByte( escape_key ) );
        net.get_current_state().push_back( Parser::UserByte( the_byte ) );
      }

      quit_sequence_started = false;

      if ( overlays.get_notification_engine().get_notification_string() == escape_key_help ) {
        overlays.get_notification_engine().set_notification_string( L"" );
      }

      continue;
    }

    quit_sequence_started
      = ( escape_key > 0 ) && ( the_byte == escape_key ) && ( lf_entered || ( !escape_requires_lf ) );
    if ( quit_sequence_started ) {
      lf_entered = false;
      overlays.get_notification_engine().set_notification_string( escape_key_help, true, false );
      continue;
    }

    lf_entered = ( ( the_byte == 0x0A )
                   || ( the_byte == 0x0D ) ); /* LineFeed, Ctrl-J, '\n' or CarriageReturn, Ctrl-M, '\r' */

    if ( the_byte == 0x0C ) { /* Ctrl-L */
      repaint_requested = true;
    }

    net.get_current_state().push_back( Parser::UserByte( the_byte ) );
  }

  return true;
}

bool STMClient::process_resize( void )
{
#ifdef _WIN32
  /* Get new console size via Windows Console API */
  short new_rows = window_rows;
  short new_cols = window_cols;
  if ( !windows_get_console_size( &new_rows, &new_cols ) ) {
    fputs( "GetConsoleScreenBufferInfo\n", stderr );
    return false;
  }
  window_rows = new_rows;
  window_cols = new_cols;

  /* tell remote emulator */
  Parser::Resize res( window_cols, window_rows );
#else
  /* get new size */
  if ( ioctl( STDIN_FILENO, TIOCGWINSZ, &window_size ) < 0 ) {
    perror( "ioctl TIOCGWINSZ" );
    return false;
  }

  /* tell remote emulator */
  Parser::Resize res( window_size.ws_col, window_size.ws_row );
#endif

  if ( !network->shutdown_in_progress() ) {
    network->get_current_state().push_back( res );
  }

  /* note remote emulator will probably reply with its own Resize to adjust our state */

  /* tell prediction engine */
  overlays.get_prediction_engine().reset();

  return true;
}

bool STMClient::main( void )
{
  /* initialize signal handling and structures */
  main_init();

  /* Drop unnecessary privileges */
#ifdef HAVE_PLEDGE
  /* OpenBSD pledge() syscall */
  if ( pledge( "stdio inet tty", NULL ) ) {
    perror( "pledge() failed" );
    exit( 1 );
  }
#endif

  /* prepare to poll for events */
  Select& sel = Select::get_instance();

  while ( 1 ) {
    try {
      output_new_frame();

      int wait_time = std::min( network->wait_time(), overlays.wait_time() );

      /* Handle startup "Connecting..." message */
      if ( still_connecting() ) {
        wait_time = std::min( 250, wait_time );
      }

      /* poll for events */
      /* network->fd() can in theory change over time */
      sel.clear_fds();
      std::vector<int> fd_list( network->fds() );
      for ( std::vector<int>::const_iterator it = fd_list.begin(); it != fd_list.end(); it++ ) {
        sel.add_fd( *it );
      }
      sel.add_fd( STDIN_FILENO );

      int active_fds = sel.select( wait_time );
      if ( active_fds < 0 ) {
        perror( "select" );
        break;
      }

      bool network_ready_to_read = false;

      for ( std::vector<int>::const_iterator it = fd_list.begin(); it != fd_list.end(); it++ ) {
        if ( sel.read( *it ) ) {
          /* packet received from the network */
          /* we only read one socket each run */
          network_ready_to_read = true;
        }
      }

      if ( network_ready_to_read ) {
        process_network_input();
      }

      if ( sel.read( STDIN_FILENO )
           && !process_user_input( STDIN_FILENO ) ) { /* input from the user needs to be fed to the network */
        if ( !network->has_remote_addr() ) {
          break;
        } else if ( !network->shutdown_in_progress() ) {
          overlays.get_notification_engine().set_notification_string( std::wstring( L"Exiting..." ), true );
          network->start_shutdown();
        }
      }

#ifdef _WIN32
      /* Windows: simulate SIGWINCH by detecting console size changes */
      {
        short new_rows = window_rows, new_cols = window_cols;
        if ( windows_get_console_size( &new_rows, &new_cols )
             && ( new_rows != window_rows || new_cols != window_cols ) ) {
          if ( !process_resize() ) {
            return false;
          }
        }
      }

      if ( sel.signal( SIGTERM ) || sel.signal( SIGINT ) ) {
        /* shutdown signal (Windows: Ctrl-C, Ctrl-Break, console close) */
        if ( !network->has_remote_addr() ) {
          break;
        } else if ( !network->shutdown_in_progress() ) {
          overlays.get_notification_engine().set_notification_string(
            std::wstring( L"Signal received, shutting down..." ), true );
          network->start_shutdown();
        }
      }
#else
      if ( sel.signal( SIGWINCH ) && !process_resize() ) { /* resize */
        return false;
      }

      if ( sel.signal( SIGCONT ) ) {
        resume();
      }

      if ( sel.signal( SIGTERM ) || sel.signal( SIGINT ) || sel.signal( SIGHUP ) || sel.signal( SIGPIPE ) ) {
        /* shutdown signal */
        if ( !network->has_remote_addr() ) {
          break;
        } else if ( !network->shutdown_in_progress() ) {
          overlays.get_notification_engine().set_notification_string(
            std::wstring( L"Signal received, shutting down..." ), true );
          network->start_shutdown();
        }
      }
#endif

      /* quit if our shutdown has been acknowledged */
      if ( network->shutdown_in_progress() && network->shutdown_acknowledged() ) {
        clean_shutdown = true;
        break;
      }

      /* quit after shutdown acknowledgement timeout */
      if ( network->shutdown_in_progress() && network->shutdown_ack_timed_out() ) {
        break;
      }

      /* quit if we received and acknowledged a shutdown request */
      if ( network->counterparty_shutdown_ack_sent() ) {
        clean_shutdown = true;
        break;
      }

      /* write diagnostic message if can't reach server */
      if ( still_connecting() && ( !network->shutdown_in_progress() )
           && ( timestamp() - network->get_latest_remote_state().timestamp > 250 ) ) {
        if ( timestamp() - network->get_latest_remote_state().timestamp > 15000 ) {
          if ( !network->shutdown_in_progress() ) {
            overlays.get_notification_engine().set_notification_string(
              std::wstring( L"Timed out waiting for server..." ), true );
            network->start_shutdown();
          }
        } else {
          overlays.get_notification_engine().set_notification_string( connecting_notification );
        }
      } else if ( ( network->get_remote_state_num() != 0 )
                  && ( overlays.get_notification_engine().get_notification_string() == connecting_notification ) ) {
        overlays.get_notification_engine().set_notification_string( L"" );
      }

      network->tick();

      std::string& send_error = network->get_send_error();
      if ( !send_error.empty() ) {
        overlays.get_notification_engine().set_network_error( send_error );
        send_error.clear();
      } else {
        overlays.get_notification_engine().clear_network_error();
      }
    } catch ( const Network::NetworkException& e ) {
      if ( !network->shutdown_in_progress() ) {
        overlays.get_notification_engine().set_network_error( e.what() );
      }

      struct timespec req;
      req.tv_sec = 0;
      req.tv_nsec = 200000000; /* 0.2 sec */
      nanosleep( &req, NULL );
      freeze_timestamp();
    } catch ( const Crypto::CryptoException& e ) {
      if ( e.fatal ) {
        throw;
      } else {
        wchar_t tmp[128];
        swprintf( tmp, 128, L"Crypto exception: %s", e.what() );
        overlays.get_notification_engine().set_notification_string( std::wstring( tmp ) );
      }
    }
  }
  return clean_shutdown;
}
