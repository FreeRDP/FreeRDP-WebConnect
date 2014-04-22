# EHS - embedded http(s) server

cmake_minimum_required(VERSION 2.8)

project("ehs")
set (SVNREV "Unknown")

# Include our extra modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

# include cmake modules
include(CheckIncludeFile)
include(CheckLibraryExists) 
include(CheckFunctionExists)
include(CheckCXXCompilerFlag)
include(CheckTypeSize)

# package naming
set(PACKAGE ${CMAKE_PROJECT_NAME})
set(PACKAGE_NAME "ehs")
set(PACKAGE_STRING "${PACKAGE_NAME}-${EHS_VERSION}")

# versioning info
set(EHS_MAJOR_VERSION 1)
set(EHS_MINOR_VERSION 5)
set(EHS_PATCH_VERSION 0)
set(EHS_VERSION
  ${EHS_MAJOR_VERSION}.${EHS_MINOR_VERSION}.${EHS_PATCH_VERSION})
set(VERSION ${EHS_VERSION})

# packe URLs
set(PACKAGE_URL "http://sourceforge.net/projects/ehs/")
set(PACKAGE_BUGREPORT "ehsdev@fritz-elfert.de")

# Default should be ON, but just to be sure
set(CMAKE_COLOR_MAKEFILE ON)

# Default to release build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

# The following code is borrowed from the FreeRDP project
# Please see github.com/FreeRDP/FreeRDP

# handle MSVC compiler options
if(MSVC)
	# default to dynamic runtime
	if(NOT DEFINED MSVC_RUNTIME)
		set(MSVC_RUNTIME "dynamic")
	endif()

	# check runtime and print messages
	if(${MSVC_RUNTIME} STREQUAL "static")
		message(STATUS "Use the MSVC static runtime option carefully!")
		message(STATUS "OpenSSL uses /MD by default, and is very picky")
		message(STATUS "Random freeing errors are a common sign of runtime issues")
	elseif(${MSVC_RUNTIME} STREQUAL "dynamic")
		message(STATUS "Using MSVC dynamic runtime")
	else()
		message(STATUS "MSVC_RUNTIME was set to an invalid value: ${MSVC_RUNTIME}")
		message(STATUS "Setting it to dynamic")
		set(MSVC_RUNTIME "dynamic")
	endif()

	# Set compiler options.
	set(variables
		CMAKE_C_FLAGS
		CMAKE_C_FLAGS_DEBUG
		CMAKE_C_FLAGS_MINSIZEREL
		CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_RELWITHDEBINFO
		CMAKE_CXX_FLAGS
		CMAKE_CXX_FLAGS_DEBUG
		CMAKE_CXX_FLAGS_MINSIZEREL
		CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_RELWITHDEBINFO)

	# set static or dynamic runtime flags
	if(${MSVC_RUNTIME} STREQUAL "static")
		message(STATUS "MSVC: using statically-linked runtime (/MT and /MTd).")
		foreach(variable ${variables})
			if(${variable} MATCHES "/MD")
				string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
			endif()
		endforeach()
	else()
		message(STATUS "MSVC: using dynamically-linked runtime (/MD and /MDd).")
		foreach(variable ${variables})
			if(${variable} MATCHES "/MT")
				string(REGEX REPLACE "/MT" "/MD" ${variable} "${${variable}}")
			endif()
		endforeach()
	endif()

	# set automatic inlining, both for __inline declared functions, as well as any that the compiler deems appropiate
	foreach(variable ${variables})
		if(${variable} MATCHES "/Ob0")
			string(REGEX REPLACE "/Ob0" "/Ob2" ${variable} "${${variable}}")
		endif()
	endforeach()

	# set warning level 2
	foreach(variable ${variables})
		if(${variable} MATCHES "/W3")
			string(REGEX REPLACE "/W3" "/W2" ${variable} "${${variable}}")
		endif()
	endforeach()

	# set same flags for both C and CXX
	set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
	set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")

	# put variables in the cmake cache
	foreach(variable ${variables})
		set(${variable} "${${variable}}" CACHE STRING "MSVC_${variable}" FORCE)
	endforeach()

	# do not add rule to re-run cmake
	if(NOT DEFINED CMAKE_SUPPRESS_REGENERATION)
		set(CMAKE_SUPPRESS_REGENERATION ON)
	endif()

	# make sure executable goes into project root binary dir
	set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})

	# add flags for debug
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		set(EHS_DEBUG 1)
	else()
		# add debugging info
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zi")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
	endif()

	# set DEPRECATED(x) to __declspec(deprecated(x))
	set(DEPRECATED "__declspec(deprecated(x))")
	set(COMPILE_WITH_SSL 1)
endif()


# handle gcc options
if(CMAKE_COMPILER_IS_GNUCC)

	# add -fPIC flag to bindhelper
	#if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "i686")
#		if(CMAKE_SIZEOF_VOID_P EQUAL 8)
#			set(BINDHELPER_LINK_FLAGS "-fPIC")
#		else()
#			set(BINDHELPER_LINK_FLAGS "-march=i686")
#		endif()
#	else()
#		
#		if(${CMAKE_VERSION} VERSION_LESS 2.8.9)
#			set(BINDHELPER_LINK_FLAGS "-fPIC")
#		endif()
#	endif()

	# check and enable C++11
	check_cxx_compiler_flag("-std=c++11" HAVE_CPLUSPLUS11)
	if (HAVE_CPLUSPLUS11)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	else()
		message(FATAL_ERROR "C++11 support is required. GCC 4.8+ is recommended.")
	endif()

	# option to enable compiler warnings
	option(GCC_ENABLE_WARNINGS "Enable gcc/g++ compiler warnings" OFF)

	if(GCC_ENABLE_WARNINGS)
		# set compiler warnings
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wchar-subscripts")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcomment")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat-security")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmissing-braces")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wparentheses")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wreturn-type")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wswitch")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wuninitialized")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wctor-dtor-privacy")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weffc++")
	endif()

	# add release/debug flags
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
	else()
		set(EHS_DEBUG 1)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
	endif()

	set(DEPRECATED "__attribute__((deprecated (x)))")
endif()

# TODO: handle Clang 

if(WIN32)
	set(_UNICODE 1)

	set(_WIN32 1)
	# Minimum Windows version (XP)
	SET(_WIN32_WINNT 0x0501)
	# find winSDK path
	find_package(WindowsSDK REQUIRED)
	include_directories(${WINSDK_INCLUDE_DIRS})
	# check for headers
	check_include_file(Windows.h HAVE_WINDOWS_H)
	check_include_file(winsock2.h HAVE_WINSOCK2_H)
endif()

if(UNIX)
	# try work-around for bug http://www.cmake.org/Bug/print_bug_page.php?bug_id=13742
	execute_process(COMMAND gcc -print-multiarch
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			OUTPUT_VARIABLE UBUNTU_MULTIARCH_INCLUDE_PATH
			ERROR_QUIET)

	if (UBUNTU_MULTIARCH_INCLUDE_PATH)
		# remove trailing newline
		STRING(REGEX REPLACE "(\r?\n)+$" "" UBUNTU_MULTIARCH_INCLUDE_PATH "${UBUNTU_MULTIARCH_INCLUDE_PATH}")
		include_directories(/usr/include/${UBUNTU_MULTIARCH_INCLUDE_PATH})
	endif()
endif()

# check for included header-files

CHECK_INCLUDE_FILE(arpa/inet.h HAVE_ARPA_INET_H)
CHECK_INCLUDE_FILE(dlfcn.h HAVE_DLFCN_H)
CHECK_INCLUDE_FILE(fcntl.h AVE_FCNTL_H)
CHECK_INCLUDE_FILE(netinet/in.h HAVE_NETINET_IN_H)
CHECK_INCLUDE_FILE(signal.h HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE(stdlib.h HAVE_STDLIB_H )
CHECK_INCLUDE_FILE(string.h HAVE_STRING_H )
CHECK_INCLUDE_FILE(strings.h HAVE_STRINGS_H )
CHECK_INCLUDE_FILE(syslog.h HAVE_SYSLOG_H )
CHECK_INCLUDE_FILE(sys/ioctl.h HAVE_SYS_IOCTL_H )
CHECK_INCLUDE_FILE(sys/resource.h HAVE_SYS_RESOURCE_H  )
CHECK_INCLUDE_FILE(sys/socket.h HAVE_SYS_SOCKET_H  )
CHECK_INCLUDE_FILE(sys/stat.h HAVE_SYS_STAT_H  )
CHECK_INCLUDE_FILE(sys/time.h HAVE_SYS_TIME_H  )
CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H  )
CHECK_INCLUDE_FILE(sys/wait.h HAVE_SYS_WAIT_H)
CHECK_INCLUDE_FILE(termios.h HAVE_TERMIOS_H  )
CHECK_INCLUDE_FILE(time.h HAVE_TIME_H  )
CHECK_INCLUDE_FILE(unistd.h HAVE_UNISTD_H  )
CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H  )
CHECK_INCLUDE_FILE(execinfo.h HAVE_EXECINFO_H  )
CHECK_INCLUDE_FILE(conio.h HAVE_CONIO_H  )
CHECK_INCLUDE_FILE(vfork.h HAVE_VFORK_H  )
CHECK_INCLUDE_FILE(memory.h HAVE_MEMORY_H)


find_file(HAVE_DWARF_H NAMES dwarf.h)
find_file(HAVE_LIBDWFL_H NAMES libdwfl.h)
find_file(HAVE_BFD_H NAMES bfd.h)


# check type sizes
set(CMAKE_EXTRA_INCLUDE_FILES "sys/types.h")
set(PID_T)
check_type_size("pid_t" PID_T_SIZE)
if(NOT HAVE_PID_T)
	set(PID_T int)
endif()
set(SIZE_T)
check_type_size("size_t" SIZE_T_SIZE)
if(NOT HAVE_SIZE_T)
	set(SIZE_T "unsigned int")
endif()
set(CMAKE_EXTRA_INCLUDE_FILES)


# linked libraries list
SET(LIBS)


option(PREFER_X86 "Prefer building for x86" ON)
if(PREFER_X86)
set(PLATFORM _X86_)
else()
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(PLATFORM _AMD64_)
else()
 set(PLATFORM _X86_)
endif()
endif()

# if win32 use PTHREADS_WIN32
if(WIN32)
	set(THREADS_USE_PTHREADS_WIN32 true)
endif(WIN32)
# find threads package
find_package(Threads REQUIRED)
if(WIN32)
	include_directories(${THREADS_PTHREADS_INCLUDE_DIR})
endif()
# CMAKE_THREAD_LIBS_INIT might also be set to -pthread, which is a compiler (and not a linker flag)
# should we add a check for that?
# add thread to libs list
set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
 set(HAVE_PTHREAD_GETW32THREADID_NP 1)
endif()

# find zlib. required by png
find_package(ZLIB REQUIRED)
include_directories(${ZLIB_INCLUDE_DIRS})
set(LIBS ${LIBS} ${ZLIB_LIBRARIES})

# find openssl 
find_package(OpenSSL REQUIRED)
message("OPENSSL INCLUDE DIR: ${OPENSSL_INCLUDE_DIR}")
include_directories(${OPENSSL_INCLUDE_DIR})

# check_include_file failes on windows. use find_file as work-around.
find_file(HAVE_OPENSSL_SSL_H
	 NAMES openssl/ssl.h 
	PATHS  ${OPENSSL_INCLUDE_DIR})

find_file(HAVE_OPENSSL_ERR_H 
	NAMES openssl/err.h 
	PATHS  ${OPENSSL_INCLUDE_DIR})

# add openssl to libs list
set(LIBS ${LIBS} ${OPENSSL_LIBRARIES})


CHECK_FUNCTION_EXISTS(memset HAVE_MEMSET)
CHECK_FUNCTION_EXISTS(select HAVE_SELECT)
CHECK_FUNCTION_EXISTS(setlocale HAVE_SETLOCALE)
CHECK_FUNCTION_EXISTS(strcasecmp HAVE_STRCASECMP)

CHECK_FUNCTION_EXISTS(pthread_getw32threadid_np HAVE_PTHREAD_GETW32THREADID_NP)
CHECK_FUNCTION_EXISTS(pthread_getw32threadhandle_np HAVE_PTHREAD_GETW32THREADHANDLE_NP)

# check for fork and vfork
# do this only for unix, as code that forks is absent in #ifdef WIN32
if(UNIX)
	CHECK_FUNCTION_EXISTS(fork HAVE_WORKING_FORK)
	CHECK_FUNCTION_EXISTS(vfork HAVE_WORKING_VFORK)
	if (NOT HAVE_WORKING_VFORK AND HAVE_WORKING_FORK)
		set(VFORK fork)
	endif()
endif()


# better check for MSVC? 
if(WIN32)
	set(STRCASE_CMP_REDEFINITION "_stricmp")
endif()
# if we're on UNIX leave it as is, even if function is not found.
if(UNIX)
	set(STRCASE_CMP_REDEFINITION "strcasecmp")
endif()

message(STATUS "Strcasecmp has been redefined to ${STRCASE_CMP_REDEFINITION}")

CHECK_FUNCTION_EXISTS(strerror HAVE_STRERROR)
CHECK_FUNCTION_EXISTS(strtoul HAVE_STRTOUL)
CHECK_FUNCTION_EXISTS(vprintf HAVE_VPRINTF)
CHECK_FUNCTION_EXISTS(inet_ntoa HAVE_INET_NTOA)

if(UNIX)
# check for __PRETTY_FUNCITON__
try_compile(HAVE_GNU_PRETTY_FUNCTION
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/cmake/pretty_func.c
	OUTPUT_VARIABLE LOG1
	)
	message(STATUS "Try_compile pretty_func.c returned ${HAVE_GNU_PRETTY_FUNCTION}")
else()
	# on WIN32 pretty_func was handled in code
	set(HAVE_GNU_PRETTY_FUNCTION 1)
endif()

# check for unique_pointer
try_compile(USE_UNIQUE_PTR 
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/cmake/unique_pointer.c
	OUTPUT_VARIABLE LOG2
	)

message(STATUS "Try_compile unique_pointer.c returned ${USE_UNIQUE_PTR}.")

if(USE_UNIQUE_PTR)
	set(USE_UNIQUE_PTR 1)
else()
	set(USE_UNIQUE_PTR 0)
endif()

# function add_found_libs_flags <library_to_search> <function_from_library>
# adds the found library to $LIBS list of linked libraries
# exits with error if not found
function(add_found_libs_flags lib libfunc)
	string(TOUPPER ${lib} libuppercase)
	CHECK_LIBRARY_EXISTS(${lib} ${libfunc} "" HAVE_LIB${libuppercase})
	if (HAVE_LIB${libuppercase})
		SET(LIBS ${LIBS} ${lib} PARENT_SCOPE)
	endif()
endfunction(add_found_libs_flags)

add_found_libs_flags(sokt socket)
add_found_libs_flags(z inflate)
add_found_libs_flags(crypto ERR_free_strings)
add_found_libs_flags(ws2_32 printf)
add_found_libs_flags(png png_read_info)


# search for tracing libs
# but do not add linker flags yet
option(PREFER_BFD "Prefer BFD over Dwarf" OFF)
check_library_exists(dw dwfl_begin "" HAVE_LIBDW)
check_library_exists(bfd bfd_openr "" HAVE_LIBBFD)
check_library_exists(iberty cplus_demangle "" HAVE_LIBIBERTY)


# Following section needs to be checked.


if(NOT HAVE_LIBDWFL_H)
	find_file(HAVE_ELFUTILS_LIBDWFL_H NAMES elfutils/libdwfl.h)
	if (HAVE_ELFUTILS_LIBDWFL_H)
		INCLUDE_DIRECTORIES(${HAVE_ELFUTILS_LIBDWFL_H})
	endif()
endif()

if(PREFER_BFD)
	if(HAVE_LIBBFD AND HAVE_BFD_H)
		set(LIBS ${LIBS} bfd)
		set(USE_BFD 1)
		set(USE_DWARF 0)
		message(STATUS "Will use libbfd for resolving symbols.")
		CHECK_FUNCTION_EXISTS(bfd_demangle HAVE_BFD_DEMANGLE)
		if(NOT HAVE_BFD_DEMANGLE)
			if (HAVE_LIBIBERTY)
				set(LIBS ${LIBS} iberty)
				message(STATUS  "Will use libiberty for tracing")
			else()
				message(FATAL_ERROR "PREFER_BFD Option set in Cache, but function bfd_demangle is not present in libbfd and libiberty is also absent. Unset option, or make sure your libbfd contains the required function or install libiberty.")
			endif(HAVE_LIBIBERTY)
		endif(NOT HAVE_BFD_DEMANGLE)
	else()
		message(SEND_ERROR "PREFER_BFD is set to ON, but libbfd (or its header) or libiberty was not found.")
		if(HAVE_LIBDW)
			set(LIBS ${LIBS} dw)
			set(USE_BFD 0)
			set(USE_DWARF 1)
			message(STATUS "Will use libdw instead.")
			if (HAVE_LIBIBERTY)
				set(LIBS ${LIBS} iberty)
				message(STAUTS "Will use libiberty for tracing.")
			endif(HAVE_LIBIBERTY)
		else()
			set(USE_BFD 0)
			set(USE_DWARF 0)
			message(STATUS "No symbol resolution or tracing will be enabled.")
		endif(HAVE_LIBDW)
	endif(HAVE_LIBBFD AND HAVE_BFD_H)
else()
	if(HAVE_LIBDW AND (HAVE_DWARF_H OR HAVE_ELFUTILS_LIBDWFL_H))
		set(LIBS ${LIBS} dw)
		set(USE_DWARF 1)
		message(STATUS "Will use libdw for resolving symbols")
		set(USE_BFD 0)
		if (HAVE_LIBIBERTY)
			set(LIBS ${LIBS} iberty)
			message(STATUS "Will use libiberty for tracing")
		endif(HAVE_LIBIBERTY)
	elseif(HAVE_LIBBFD AND HAVE_BFD_H)
		CHECK_FUNCTION_EXISTS(bfd_demangle HAVE_BFD_DEMANGLE)
		if (NOT (HAVE_BFD_DEMANGLE OR HAVE_LIBIBERTY))
			set(USE_BFD 0)
			set(USE_DWARF 0)
			message(STATUS "Libbfd detected, but function bfd_demangle is not present and libiberty is also absent. Make sure your libbfd contains the required function or install libiberty to enable symbol resolution and tracing.")
		else()
			set(LIBS ${LIBS} bfd)
			message(STATUS "Will use libbfd for symbol resolution.")
			set(USE_BFD 1)
			if (HAVE_LIBIBERTY AND (NOT HAVE_BFD_DEMANGLE))
				set(LIBS ${LIBS} iberty)
				message(STATUS "Will use libibert for tracing.")
			endif(HAVE_LIBIBERTY AND (NOT HAVE_BFD_DEMANGLE))
		endif(NOT (HAVE_BFD_DEMANGLE OR HAVE_LIBIBERTY))
	endif(HAVE_LIBDW AND (HAVE_DWARF_H OR HAVE_ELFUTILS_LIBDWFL_H))
endif(PREFER_BFD)
# endsection


# search for boost libs
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(BOOST_ALL_DYN_LINK ON)
find_package(Boost 1.48 REQUIRED filesystem system regex program_options)

set(HAVE_BOOST 1)
set(HAVE_BOOST_LOCK_GUARD 1)
set(HAVE_BOOST_REGEX_HPP 1)
set(HAVE_BOOST_PROGRAM_OPTIONS_HPP 1)
set(HAVE_BOOST_SYSTEM_ERROR_CODE_HPP 1)
set(HAVE_BOOST_FILESYSTEM_PATH_HPP 1)
#For the moment EHS_DEBUG introduces no target architecture error on winnt
set(EHS_DEBUG 0)

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

# create config.h from config.h.cmake
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
		${CMAKE_CURRENT_BINARY_DIR}/config.h)

# create ehstypes.h from ehstypes.h.in
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/ehstypes.h.cmake
		${CMAKE_CURRENT_SOURCE_DIR}/ehstypes.h
		)

# add config.h location to include search path
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# add preprocessor definition
add_definitions(-DHAVE_CONFIG_H)

# Variables used for install
set(SYSCONFDIR)
set(LIBEXECDIR)
set(DATADIR)

# install section
if(UNIX)
	INCLUDE(GNUInstallDirs)
elseif(WIN32)
	# In windows set a single install path
	set(SYSCONFDIR "${CMAKE_INSTALL_PREFIX}")
	set(LIBEXECDIR "${CMAKE_INSTALL_PREFIX}")
	set(DATADIR "${CMAKE_INSTALL_PREFIX}")
endif()

# set full paths for installed objects


# Workaround for libexecdir on debian
#if (EXISTS "/etc/debian_version") 
#  set(CMAKE_INSTALL_LIBEXECDIR ${CMAKE_INSTALL_LIBDIR})
#  set(CMAKE_INSTALL_FULL_LIBEXECDIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBEXECDIR}")
#endif()

#set(pkglibexecdir "${CMAKE_INSTALL_FULL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")


include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(EHS_SOURCES datum.cpp dynamicssllocking.cpp ehs.cpp formvalue.cpp httprequest.cpp
			httpresponse.cpp osdep.cpp securesocket.cpp socket.cpp sslerror.cpp staticssllocking.cpp)

if (WIN32)
	# in order for header files to appear in VS solution, add them to the sources list
	set(EHS_SOURCES "${EHS_SOURCES}" ${CMAKE_CURRENT_BINARY_DIR}/config.h contentdisposition.h datum.h
			debug.h dynamicssllocking.h ehs.h ehsconnection.h ehsserver.h ehstypes.h httprequest.h
			httpresponse.h mutexhelper.h networkabstraction.h securesocket.h socket.h sslerror.h 
			staticssllocking.h)
endif()

add_library(ehs STATIC ${EHS_SOURCES})

#target_link_libraries(ehs "-fPIC")

target_link_libraries(ehs ${LIBS})

set_target_properties(ehs PROPERTIES VERSION ${EHS_MAJOR_VERSION} SOVERSION ${EHS_VERSION})