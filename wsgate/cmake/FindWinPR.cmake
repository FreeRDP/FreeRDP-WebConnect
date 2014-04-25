# FindWinPR package
#
# Set WINPR_ROOT_DIR to guide the package to the winpr installation.
# The script will set WINPR_INCLUDE_DIR pointing to the location of header files.
# and WINPR_LIBRARIES will be a list that can be passed to taget_link_libraries.
# The script checks for the winpr/input.h header file only.
# It searches for winpr-input.lib libwinpr-input.so and libwinpr-input.a,
# But will only add one of them to the link list.

# winpr is part of the freerdp project and is shipped together with it
if(UNIX)
	find_package(PkgConfig)
	pkg_check_modules(_WINPR QUIET freerdp)
endif()

if (WIN32)
	set(_WINPR_ROOT_HINTS
		${WINPR_ROOT_DIR}
		ENV WINPR_ROOT_DIR
		ENV FREERDP_ROOT_DIR
	)
	file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)
	set(_WINPR_ROOT_PATHS
		"${_programfiles}/FreeRDP"
		"C:/FreeRDP"
	)
	unset(_programfiles)
	set(_WINPR_ROOT_HINTS_AND_PATHS
		HINTS ${_WINPR_ROOT_HINTS}
		PATHS ${_WINPR_ROOT_PATHS}
	)
else()
	set(_WINPR_ROOT_HINTS
    		${WINPR_ROOT_DIR}
    		ENV WINPR_ROOT_DIR
    		ENV FREERDP_ROOT_DIR
    	)
endif(WIN32)

find_path(WINPR_INCLUDE_DIR
	NAMES
	 winpr/input.h
	 HINTS
	  ${_WINPR_INCLUDEDIR}
	 ${_WINPR_ROOT_HINTS_AND_PATHS}
	 PATH_SUFFIXES
	  include
)

if(WIN32 AND NOT CYGWIN)
	if(MSVC)
		SET(CMAKE_FIND_LIBRARY_PREFIXES "")
		SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")

		find_library(LIB_WINPR_INPUT
			NAMES
			 winpr-input
			${_WINPR_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			"lib"
			"VC"
			"lib/winpr"
		)

	endif(MSVC)
elseif(UNIX)
	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
	find_library(LIB_WINPR_INPUT
			NAMES
			 winpr-input
			HINTS
			 ${_WINPR_LIBDIR}
			${_WINPR_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			 lib
			)
endif(WIN32 AND NOT CYGWIN)

mark_as_advanced(LIB_WINPR_INPUT)

set(WINPR_LIBRARIES ${LIB_WINPR_INPUT})

# WINPR version is actually freerdp version
if (WINPR_INCLUDE_DIR)
	if(_WINPR_VERSION)
		set(WINPR_VERSION "${_WINPR_VERSION}")
	elseif(WINPR_INCLUDE_DIR AND EXISTS "${WINPR_INCLUDE_DIR}/freerdp/version.h")
		file(STRINGS "${WINPR_INCLUDE_DIR}/freerdp/version.h" _freerdp_major_version
		REGEX "^#define[\t ]+FREERDP_VERSION_MAJOR[\t ]+[0-9]")

		string(REGEX REPLACE "^#define[\t ]+FREERDP_VERSION_MAJOR[\t ]+" "" 
			WINPR_MAJOR_VERSION ${_freerdp_major_version})

		file(STRINGS "${WINPR_INCLUDE_DIR}/freerdp/version.h" _freerdp_minor_version
		REGEX "^#define[\t ]+FREERDP_VERSION_MINOR[\t ]+[0-9]")

		string(REGEX REPLACE "^#define[\t ]+FREERDP_VERSION_MINOR[\t ]+" "" 
			WINPR_MINOR_VERSION ${_freerdp_minor_version})

		file(STRINGS "${WINPR_INCLUDE_DIR}/freerdp/version.h" _freerdp_revision_version
		REGEX "^#define[\t ]+FREERDP_VERSION_REVISION[\t ]+[0-9]")

		string(REGEX REPLACE "^#define[\t ]+FREERDP_VERSION_REVISION[\t ]+" "" 
			WINPR_REVISION_VERSION ${_freerdp_revision_version})

		SET(WINPR_VERSION "${WINPR_MAJOR_VERSION}.${WINPR_MINOR_VERSION}.${WINPR_REVISION_VERSION}")
	endif(_WINPR_VERSION)
endif(WINPR_INCLUDE_DIR)

if(WINPR_VERSION)
	find_package_handle_standard_args(WinPR
		REQUIRED_VARS
		 WINPR_LIBRARIES
		 WINPR_INCLUDE_DIR
		 VERSION_VAR
		  WINPR_VERSION
		 FAIL_MESSAGE
		 "Could NOT find WinPR, try to set the path to WinPR root folder in the system variable WINPR_ROOT_DIR"
	)
else()
	find_package_handle_standard_args(FreeRDP "Could NOT find WinPR, try to set the path to FreeRDP root folder in the system variable WINPR_ROOT_DIR"
		WINPR_LIBRARIES
		WINPR_INCLUDE_DIR
	)
endif(WINPR_VERSION)

mark_as_advanced(WINPR_INCLUDE_DIR WINPR_LIBRARIES)