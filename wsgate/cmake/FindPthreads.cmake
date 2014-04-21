# FindPthread package
# 
# Set PTHREAD_ROOT_DIR to guide the package to the pthread installation.
# The script will set PTHREAD_INCLUDE_DIR pointing to the location of header files.
# and PTHREAD_LIBRARIES will be a list that can be passed to taget_link_libraries.
# The script checks for the pthread.h header file only.

# The following is taken from http://trac.research.cc.gatech.edu/fastlab/browser/trunk/CMake/FindPthreads.cmake?rev=11827

# This module responds to the PTHREADS_EXCEPTION_SCHEME
# variable on Win32 to allow the user to control the
# library linked against.  The Pthreads-win32 port
# provides the ability to link against a version of the
# library with exception handling.  IT IS NOT RECOMMENDED
# THAT YOU USE THIS because most POSIX thread implementations
# do not support stack unwinding.
#
#  PTHREADS_EXCEPTION_SCHEME
#      C  = no exceptions (default)
#         (NOTE: This is the default scheme on most POSIX thread
#          implementations and what you should probably be using)
#      CE = C++ Exception Handling
#      SE = Structure Exception Handling (MSVC only)
#

#
# Define a default exception scheme to link against
# and validate user choice.
#

IF(NOT DEFINED PTHREADS_EXCEPTION_SCHEME)
    # Assign default if needed
    SET(PTHREADS_EXCEPTION_SCHEME "C")
ELSE(NOT DEFINED PTHREADS_EXCEPTION_SCHEME)
    # Validate
    IF(NOT PTHREADS_EXCEPTION_SCHEME STREQUAL "C" AND
       NOT PTHREADS_EXCEPTION_SCHEME STREQUAL "CE" AND
       NOT PTHREADS_EXCEPTION_SCHEME STREQUAL "SE")

    MESSAGE(FATAL_ERROR "See documentation for FindPthreads.cmake, only C, CE, and SE modes are allowed")

    ENDIF(NOT PTHREADS_EXCEPTION_SCHEME STREQUAL "C" AND
          NOT PTHREADS_EXCEPTION_SCHEME STREQUAL "CE" AND
          NOT PTHREADS_EXCEPTION_SCHEME STREQUAL "SE")

     IF(NOT MSVC AND PTHREADS_EXCEPTION_SCHEME STREQUAL "SE")
         MESSAGE(FATAL_ERROR "Structured Exception Handling is only allowed for MSVC")
     ENDIF(NOT MSVC AND PTHREADS_EXCEPTION_SCHEME STREQUAL "SE")

ENDIF(NOT DEFINED PTHREADS_EXCEPTION_SCHEME) 

SET(HOST_CPU ${CMAKE_HOST_SYSTEM_PROCESSOR})
STRING(TOLOWER ${HOST_CPU} HOST_CPU)

IF(HOST_CPU MATCHES "amd64")
SET(HOST_CPU "x64")
ELSEIF(HOST_CPU MATCHES "i.86")
SET(HOST_CPU "x86")
ENDIF(HOST_CPU MATCHES "amd64")

if(UNIX)
	find_package(PkgConfig)
	pkg_check_modules(_PTHREAD QUIET pthread)
endif()

if (WIN32)
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
else()
	set(_PTHREAD_ROOT_HINTS
    		${PTHREAD_ROOT_DIR}
    		ENV PTHREAD_ROOT_DIR
    	)
endif(WIN32)

find_path(PTHREAD_INCLUDE_DIR
	NAMES
	 pthread.h
	 HINTS
	  ${_PTHREAD_INCLUDEDIR}
	 ${_PTHREAD_ROOT_HINTS_AND_PATHS}
	 PATH_SUFFIXES
	  include
	Pre-built.2/include
)


SET(names)
IF(MSVC)
    SET(names
            pthreadV${PTHREADS_EXCEPTION_SCHEME}2
            pthread
    )
ELSEIF(MINGW)
    SET(names
            pthreadG${PTHREADS_EXCEPTION_SCHEME}2
            pthread
    )
ELSE(MSVC) # Unix / Cygwin / Apple
    SET(names pthread)
ENDIF(MSVC)

if(WIN32 AND NOT CYGWIN)
	if(MSVC)
		SET(CMAKE_FIND_LIBRARY_PREFIXES "")
		SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")

		find_library(LIB_PTHREAD
			NAMES
			 ${names}
			${_PTHREAD_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			"lib"
			"lib/${HOST_CPU}"
			"pre-built.2/lib/${HOST_CPU}"
		)

	endif(MSVC)
elseif(UNIX)
	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
	find_library(LIB_PTHREAD_CORE
			NAMES
			 ${names}
			HINTS
			 ${_PTHREAD_LIBDIR}
			${_PTHREAD_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			 lib
			)
endif(WIN32 AND NOT CYGWIN)

mark_as_advanced(LIB_PTHREAD)

set(PTHREAD_LIBRARIES ${LIB_PTHREAD})

find_package_handle_standard_args(Pthread
	REQUIRED_VARS
	 PTHREAD_LIBRARIES
	 PTHREAD_INCLUDE_DIR
	 FAIL_MESSAGE
	 "Could NOT find Pthread, try to set the path to Pthread root folder in the system variable PTHREAD_ROOT_DIR"
)

mark_as_advanced(PTHREAD_INCLUDE_DIR PTHREAD_LIBRARIES)