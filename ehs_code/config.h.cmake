/* Whether to compile with SSL support or not */
#cmakedefine COMPILE_WITH_SSL 1

/* GIT revision */
#cmakedefine SVNREV "@SVNREV@"

/* Define to 1 if you have the <arpa/inet.h> header file. */
#cmakedefine HAVE_ARPA_INET_H 1

/* Define to 1 if you have the `bfd_demangle' function. */
#cmakedefine HAVE_BFD_DEMANGLE 1

/* Defined if the requested minimum BOOST version is satisfied */
#cmakedefine HAVE_BOOST 1

/* Define to 1 if you have <boost/filesystem/path.hpp> */
#cmakedefine HAVE_BOOST_FILESYSTEM_PATH_HPP 1

/* Define if boost::lock_guard is available */
#cmakedefine HAVE_BOOST_LOCK_GUARD 1

/* Define to 1 if you have <boost/program_options.hpp> */
#cmakedefine HAVE_BOOST_PROGRAM_OPTIONS_HPP 1

/* Define to 1 if you have <boost/regex.hpp> */
#cmakedefine HAVE_BOOST_REGEX_HPP 1

/* Define to 1 if you have <boost/system/error_code.hpp> */
#cmakedefine HAVE_BOOST_SYSTEM_ERROR_CODE_HPP 1

/* Define to 1 if you have the <conio.h> header file. */
#cmakedefine HAVE_CONIO_H 1

/* Define to 1 if you have the <cpprest/json.h> header file. */
#cmakedefine HAVE_CPPREST_JSON_H 1

/* Define to 1 if you have the <demangle.h> header file. */
#cmakedefine HAVE_DEMANGLE_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
#cmakedefine HAVE_DOPRNT 1

/* Define to 1 if you have the <dwarf.h> header file. */
#cmakedefine HAVE_DWARF_H 1

/* Define to 1 if you have the <ehs.h> header file. */
#cmakedefine HAVE_EHS_H 1

/* Define to 1 if you have the <elfutils/libdwfl.h> header file. */
#cmakedefine HAVE_ELFUTILS_LIBDWFL_H 1

/* Define to 1 if you have the <execinfo.h> header file. */
#cmakedefine HAVE_EXECINFO_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H 1

/* Define to 1 if you have the `fork' function. */
#cmakedefine HAVE_FORK 1

/* Define if the compiler supports GNU-style __PRETTY_FUNCTION__. */
#cmakedefine HAVE_GNU_PRETTY_FUNCTION 1

/* Define if the compiler supports GNU-style variadic macros. */
#cmakedefine HAVE_GNU_VAMACROS 1

/* Define to 1 if you have the `inet_ntoa' function. */
#cmakedefine HAVE_INET_NTOA 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <libdwfl.h> header file. */
#cmakedefine HAVE_LIBDWFL_H 1

/* Define, if libiberty is available */
#cmakedefine HAVE_LIBIBERTY 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#cmakedefine HAVE_MEMSET 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#cmakedefine HAVE_NETINET_IN_H 1

/* Define to 1 if you have the <openssl/err.h> header file. */
#cmakedefine HAVE_OPENSSL_ERR_H 1

/* Define to 1 if you have the <openssl/ssl.h> header file. */
#cmakedefine HAVE_OPENSSL_SSL_H 1

/* Define to 1 if you have the `pthread_getw32threadhandle_np' function. */
#cmakedefine HAVE_PTHREAD_GETW32THREADHANDLE_NP 1

/* Define to 1 if you have the `pthread_getw32threadid_np' function. */
#cmakedefine HAVE_PTHREAD_GETW32THREADID_NP 1

/* Define to 1 if you have the `setlocale' function. */
#cmakedefine HAVE_SETLOCALE 1

/* Define to 1 if you have the <signal.h> header file. */
#cmakedefine HAVE_SIGNAL_H 1

/* Define to 1 if you have the `socket' function. */
#cmakedefine HAVE_SOCKET 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#cmakedefine HAVE_STRCASECMP 1

/* Redefinition of strcasecmp for WIN32 */
#define strcasecmp @STRCASE_CMP_REDEFINITION@

/* Redefinition of __attribute__ for WIN32 */
#ifndef __GNUC__ 
#define  __attribute__(x)  /*NOTHING*/ 
#endif

/*Using boost::throw_exception and not our own*/
#define _CPPUNWIND 1

/* Define to 1 if you have the `strerror' function. */
#cmakedefine HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the `strtoul' function. */
#cmakedefine HAVE_STRTOUL 1

/* Define to 1 if you have the <syslog.h> header file. */
#cmakedefine HAVE_SYSLOG_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#cmakedefine HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#cmakedefine HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#cmakedefine HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#cmakedefine HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/wait.h> header file. */
#cmakedefine HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the <termios.h> header file. */
#cmakedefine HAVE_TERMIOS_H 1

/* Define to 1 if you have the <time.h> header file. */
#cmakedefine HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 if you have the `vfork' function. */
#cmakedefine HAVE_VFORK 1

/* Define to 1 if you have the <vfork.h> header file. */
#cmakedefine HAVE_VFORK_H 1

/* Define to 1 if you have the `vprintf' function. */
#cmakedefine HAVE_VPRINTF 1

/* Define to 1 if you have the <windows.h> header file. */
#cmakedefine HAVE_WINDOWS_H 1

/* Define to 1 if you have the <winsock2.h> header file. */
#cmakedefine HAVE_WINSOCK2_H 1

/* Define to 1 if `fork' works. */
#cmakedefine HAVE_WORKING_FORK 1

/* Define to 1 if `vfork' works. */
#cmakedefine HAVE_WORKING_VFORK 1

/* Define to 1 if the system has the type `_Bool'. */
#cmakedefine HAVE__BOOL 1

/* Name of package */
#cmakedefine PACKAGE "@PACKAGE@"

/* Define to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT "@PACKAGE_BUGREPORT@"

/* Define to the full name of this package. */
#cmakedefine PACKAGE_NAME "@PACKAGE_NAME@"

/* Define to the full name and version of this package. */
#cmakedefine PACKAGE_STRING "@PACKAGE_STRING@"

/* Define to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME "@PACKAGE_TARNAME@"

/* Define to the home page for this package. */
#cmakedefine PACKAGE_URL "@PACKAGE_URL@"

/* Define to the version of this package. */
#cmakedefine PACKAGE_VERSION "@PACKAGE_VERSION@"

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* Define, if using bfd for resolving symbols */
#cmakedefine USE_BFD 1

/* Define, if using libdw for resolving symbols */
#cmakedefine USE_DWARF 1

/* Version number of package */
#cmakedefine VERSION "@VERSION@"

/* Whether to compile with debugging information */
#cmakedefine EHS_DEBUG 1

/* Select wide string variants */
#cmakedefine _UNICODE 1

/*set platform*/
#cmakedefine @PLATFORM@

/* Minimum Windows version (XP) */
#cmakedefine _WIN32_WINNT @_WIN32_WINNT@

#cmakedefine WIN32 1
#cmakedefine _WIN32 1

/* Define to empty if `const' does not conform to ANSI C. */
#cmakedefine const 1

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus 1
#cmakedefine inline 1
#endif 1

#define DEPRECATED(x) @DEPRECATED@