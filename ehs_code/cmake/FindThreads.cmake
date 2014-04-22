# Updated FindThreads.cmake that supports pthread-win32
# Downloaded from http://www.vtk.org/Bug/bug_view_advanced_page.php?bug_id=6399

# Use PTHREAD_ROOT_DIR to help it find the libraries.

# - This module determines the thread library of the system.
#
# The following variables are set
#  CMAKE_THREAD_LIBS_INIT     - the thread library
#  CMAKE_USE_SPROC_INIT       - are we using sproc?
#  CMAKE_USE_WIN32_THREADS_INIT - using WIN32 threads?
#  CMAKE_USE_PTHREADS_INIT    - are we using pthreads
#  CMAKE_HP_PTHREADS_INIT     - are we using hp pthreads
#
# If use of pthreads-win32 is desired, the following variables
# can be set.
#
#  THREADS_USE_PTHREADS_WIN32 -
#    Setting this to true searches for the pthreads-win32
#    port (since CMake 2.8.0)
#
#  THREADS_PTHREADS_WIN32_EXCEPTION_SCHEME
#      C  = no exceptions (default)
#         (NOTE: This is the default scheme on most POSIX thread
#          implementations and what you should probably be using)
#      CE = C++ Exception Handling
#      SE = Structure Exception Handling (MSVC only)
#      (NOTE: Changing this option from the default may affect
#       the portability of your application.  See pthreads-win32
#       documentation for more details.)
#
#======================================================
# Example usage where threading library
# is provided by the system:
#
#   find_package(Threads REQUIRED)
#   add_executable(foo foo.cc)
#   target_link_libraries(foo ${CMAKE_THREAD_LIBS_INIT})
#
# Example usage if pthreads-win32 is desired on Windows
# or a system provided thread library:
#
#   set(THREADS_USE_PTHREADS_WIN32 true)
#   find_package(Threads REQUIRED)
#   include_directories(${THREADS_PTHREADS_INCLUDE_DIR})
#
#   add_executable(foo foo.cc)
#   target_link_libraries(foo ${CMAKE_THREAD_LIBS_INIT})
#

INCLUDE (CheckIncludeFiles)
INCLUDE (CheckLibraryExists)
SET(Threads_FOUND FALSE)

if(UNIX)
  find_package(PkgConfig)
  pkg_check_modules(_PTHREAD QUIET pthread)
endif()

IF(WIN32 AND NOT CYGWIN AND THREADS_USE_PTHREADS_WIN32)
  SET(_Threads_ptwin32 true)
ENDIF()

SET(HOST_CPU ${CMAKE_HOST_SYSTEM_PROCESSOR})
STRING(TOLOWER ${HOST_CPU} HOST_CPU)

IF(HOST_CPU MATCHES "amd64")
SET(HOST_CPU "x64")
ELSEIF(HOST_CPU MATCHES "i.86")
SET(HOST_CPU "x86")
ENDIF(HOST_CPU MATCHES "amd64")

#only for freerdp
IF(WIN32)
    SET(HOST_CPU "x86")
ENDIF(WIN32)

# Do we have sproc?
IF(CMAKE_SYSTEM MATCHES IRIX)
  CHECK_INCLUDE_FILES("sys/types.h;sys/prctl.h"  CMAKE_HAVE_SPROC_H)
ENDIF()

IF(CMAKE_HAVE_SPROC_H)
  # We have sproc
  SET(CMAKE_USE_SPROC_INIT 1)

ELSEIF(_Threads_ptwin32)

  IF(NOT DEFINED THREADS_PTHREADS_WIN32_EXCEPTION_SCHEME)
    # Assign the default scheme
    SET(THREADS_PTHREADS_WIN32_EXCEPTION_SCHEME "C")
  ELSE()
    # Validate the scheme specified by the user
    IF(NOT THREADS_PTHREADS_WIN32_EXCEPTION_SCHEME STREQUAL "C" AND
       NOT THREADS_PTHREADS_WIN32_EXCEPTION_SCHEME STREQUAL "CE" AND
       NOT THREADS_PTHREADS_WIN32_EXCEPTION_SCHEME STREQUAL "SE")
         MESSAGE(FATAL_ERROR "See documentation for FindPthreads.cmake, only C, CE, and SE modes are allowed")
    ENDIF()
    IF(NOT MSVC AND THREADS_PTHREADS_WIN32_EXCEPTION_SCHEME STREQUAL "SE")
      MESSAGE(FATAL_ERROR "Structured Exception Handling is only allowed for MSVC")
    ENDIF(NOT MSVC AND THREADS_PTHREADS_WIN32_EXCEPTION_SCHEME STREQUAL "SE")
  ENDIF()


  set(_PTHREAD_ROOT_HINTS
    ${PTHREAD_ROOT_DIR}
    # could not find any registry keys related to pthread
    ENV PTHREAD_ROOT_DIR
  )
  file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)
  set(_PTHREAD_ROOT_PATHS
		"${_programfiles}/pthread"
		"${_programfiles}/pthread-w32"
		"${_programfiles}/pthread-win32"
		"${_programfiles}/pthreads"
		"${_programfiles}/pthreads-w32"
		"${_programfiles}/pthreads-win32"
		"C:/pthread"
		"C:/pthread-w32"
		"C:/pthread-win32"
		"C:/pthreads"
		"C:/pthreads-w32"
		"C:/pthreads-win32"
  )
  unset(_programfiles)
  set(_PTHREAD_ROOT_HINTS_AND_PATHS
    HINTS ${_PTHREAD_ROOT_HINTS}
    PATHS ${_PTHREAD_ROOT_PATHS}
  )

  find_path(THREADS_PTHREADS_INCLUDE_DIR
    NAMES
     pthread.h
     HINTS
      ${_PTHREAD_INCLUDEDIR}
     ${_PTHREAD_ROOT_HINTS_AND_PATHS}
     PATH_SUFFIXES
      include
	Pre-built.2/include
  )
  
  # Determine the library filename
  IF(MSVC)
    SET(_Threads_pthreads_libname
        pthreadV${THREADS_PTHREADS_WIN32_EXCEPTION_SCHEME}2)
  ELSEIF(MINGW)
    SET(_Threads_pthreads_libname
        pthreadG${THREADS_PTHREADS_WIN32_EXCEPTION_SCHEME}2)
  ELSE()
    MESSAGE(FATAL_ERROR "This should never happen")
  ENDIF()


  FIND_LIBRARY(THREADS_PTHREADS_WIN32_LIBRARY
              NAMES ${_Threads_pthreads_libname}
              ${_PTHREAD_ROOT_HINTS_AND_PATHS}
              PATH_SUFFIXES
               "lib"
               "lib/${HOST_CPU}"
               "pre-built.2/lib/${HOST_CPU}"
              DOC "The Portable Threads Library for Win32"
              )

  IF(THREADS_PTHREADS_INCLUDE_DIR AND THREADS_PTHREADS_WIN32_LIBRARY)
    MARK_AS_ADVANCED(THREADS_PTHREADS_INCLUDE_DIR)
    SET(CMAKE_THREAD_LIBS_INIT ${THREADS_PTHREADS_WIN32_LIBRARY})
    SET(CMAKE_HAVE_THREADS_LIBRARY 1)
    SET(Threads_FOUND TRUE)
  ENDIF()

  MARK_AS_ADVANCED(THREADS_PTHREADS_WIN32_LIBRARY)

ELSE()
  # Do we have pthreads?
  FIND_PATH( CMAKE_HAVE_PTHREAD_H 
              NAMES 
            pthread.h
            PATH_SUFFIXES
             include
  )
  IF(CMAKE_HAVE_PTHREAD_H)

    #
    # We have pthread.h
    # Let's check for the library now.
    #
    SET(CMAKE_HAVE_THREADS_LIBRARY)
    IF(NOT THREADS_HAVE_PTHREAD_ARG)

      # Do we have -lpthreads
      CHECK_LIBRARY_EXISTS(pthreads pthread_create "" CMAKE_HAVE_PTHREADS_CREATE)
      IF(CMAKE_HAVE_PTHREADS_CREATE)
        SET(CMAKE_THREAD_LIBS_INIT "-lpthreads")
        SET(CMAKE_HAVE_THREADS_LIBRARY 1)
        SET(Threads_FOUND TRUE)
      ENDIF()

      # Ok, how about -lpthread
      CHECK_LIBRARY_EXISTS(pthread pthread_create "" CMAKE_HAVE_PTHREAD_CREATE)
      IF(CMAKE_HAVE_PTHREAD_CREATE)
        SET(CMAKE_THREAD_LIBS_INIT "-lpthread")
        SET(Threads_FOUND TRUE)
        SET(CMAKE_HAVE_THREADS_LIBRARY 1)
      ENDIF()

      IF(CMAKE_SYSTEM MATCHES "SunOS.*")
        # On sun also check for -lthread
        CHECK_LIBRARY_EXISTS(thread thr_create "" CMAKE_HAVE_THR_CREATE)
        IF(CMAKE_HAVE_THR_CREATE)
          SET(CMAKE_THREAD_LIBS_INIT "-lthread")
          SET(CMAKE_HAVE_THREADS_LIBRARY 1)
          SET(Threads_FOUND TRUE)
        ENDIF()
      ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")

    ENDIF(NOT THREADS_HAVE_PTHREAD_ARG)

    IF(NOT CMAKE_HAVE_THREADS_LIBRARY)
      # If we did not found -lpthread, -lpthread, or -lthread, look for -pthread
      include(CheckCXXCompilerFlag)

      check_cxx_compiler_flag("-pthread" THREADS_HAVE_PTHREAD_ARG)
      IF(THREADS_HAVE_PTHREAD_ARG)
        SET(Threads_FOUND TRUE)
        SET(CMAKE_THREAD_LIBS_INIT "-pthread")
      ENDIF()

    ENDIF(NOT CMAKE_HAVE_THREADS_LIBRARY)
  ENDIF(CMAKE_HAVE_PTHREAD_H)
ENDIF()

IF(CMAKE_THREAD_LIBS_INIT)
  SET(CMAKE_USE_PTHREADS_INIT 1)
  SET(Threads_FOUND TRUE)
ENDIF()

IF(CMAKE_SYSTEM MATCHES "Windows"
   AND NOT THREADS_USE_PTHREADS_WIN32)
  SET(CMAKE_USE_WIN32_THREADS_INIT 1)
  SET(Threads_FOUND TRUE)
ENDIF()

IF(CMAKE_USE_PTHREADS_INIT)
  IF(CMAKE_SYSTEM MATCHES "HP-UX-*")
    # Use libcma if it exists and can be used.  It provides more
    # symbols than the plain pthread library.  CMA threads
    # have actually been deprecated:
    #   http://docs.hp.com/en/B3920-90091/ch12s03.html#d0e11395
    #   http://docs.hp.com/en/947/d8.html
    # but we need to maintain compatibility here.
    # The CMAKE_HP_PTHREADS setting actually indicates whether CMA threads
    # are available.
    CHECK_LIBRARY_EXISTS(cma pthread_attr_create "" CMAKE_HAVE_HP_CMA)
    IF(CMAKE_HAVE_HP_CMA)
      SET(CMAKE_THREAD_LIBS_INIT "-lcma")
      SET(CMAKE_HP_PTHREADS_INIT 1)
      SET(Threads_FOUND TRUE)
    ENDIF(CMAKE_HAVE_HP_CMA)
    SET(CMAKE_USE_PTHREADS_INIT 1)
  ENDIF()

  IF(CMAKE_SYSTEM MATCHES "OSF1-V*")
    SET(CMAKE_USE_PTHREADS_INIT 0)
    SET(CMAKE_THREAD_LIBS_INIT )
  ENDIF()

  IF(CMAKE_SYSTEM MATCHES "CYGWIN_NT*")
    SET(CMAKE_USE_PTHREADS_INIT 1)
    SET(Threads_FOUND TRUE)
    SET(CMAKE_THREAD_LIBS_INIT )
    SET(CMAKE_USE_WIN32_THREADS_INIT 0)
  ENDIF()
ENDIF(CMAKE_USE_PTHREADS_INIT)

message("Include dir: ")
INCLUDE(FindPackageHandleStandardArgs)
IF(_Threads_ptwin32)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Threads DEFAULT_MSG
    THREADS_PTHREADS_WIN32_LIBRARY THREADS_PTHREADS_INCLUDE_DIR)
ELSE()
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Threads DEFAULT_MSG Threads_FOUND)
ENDIF()
