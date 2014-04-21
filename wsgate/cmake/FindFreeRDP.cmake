# FindFreeRDP package
#
# Set FREERDP_ROOT_DIR to guide the package to the freerdp installation.
# The script will set FREERDP_INCLUDE_DIR pointing to the location of header files.
# and FREERDP_LIBRARIES will be a list that can be passed to taget_link_libraries.
# The script checks for the freerdp/freerdp.h header file only.
# It searches for freerdp-core.lib freerdp-core.so freerdp-core.a and will add one of them to the link list.
# It searches for freerdp-codec.lib freerdp-codec.so freerdp-codec.a and will add one of them to the link list.
# It searches for freerdp-gdi.lib freerdp-gdi.so freerdp-gdi.a and will add one of them to the link list.
# It searches for freerdp-cache.lib freerdp-cache.so freerdp-cache.a and will add one of them to the link list.


if(UNIX)
	find_package(PkgConfig)
	pkg_check_modules(_FREERDP QUIET freerdp)
endif()

if (WIN32)
	set(_FREERDP_ROOT_HINTS
		${FREERDP_ROOT_DIR}
		ENV FREERDP_ROOT_DIR
	)
	file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)
	set(_FREERDP_ROOT_PATHS
		"${_programfiles}/FreeRDP"
		"C:/FreeRDP"
	)
	unset(_programfiles)
	set(_FREERDP_ROOT_HINTS_AND_PATHS
		HINTS ${_FREERDP_ROOT_HINTS}
		PATHS ${_FREERDP_ROOT_PATHS}
	)
else()
	set(_FREERDP_ROOT_HINTS
    		${FREERDP_ROOT_DIR}
    		ENV FREERDP_ROOT_DIR
    	)
endif(WIN32)

find_path(FREERDP_INCLUDE_DIR
	NAMES
	 freerdp/freerdp.h
	 HINTS
	  ${_FREERDP_INCLUDEDIR}
	 ${_FREERDP_ROOT_HINTS_AND_PATHS}
	 PATH_SUFFIXES
	  include
)

if(WIN32 AND NOT CYGWIN)
	if(MSVC)
		SET(CMAKE_FIND_LIBRARY_PREFIXES "")
		SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")

		find_library(LIB_FREERDP_CORE
			NAMES
			 freerdp-core
			${_FREERDP_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			"lib"
			"VC"
			"lib/freerdp"
		)
		find_library(LIB_FREERDP_CODEC
			NAMES
			 freerdp-codec
			${_FREERDP_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			"lib"
			"VC"
			"lib/freerdp"
		)
		find_library(LIB_FREERDP_GDI
			NAMES
			 freerdp-gdi
			${_FREERDP_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			"lib"
			"VC"
			"lib/freerdp"
		)
		find_library(LIB_FREERDP_CACHE
			NAMES
			 freerdp-cache
			${_FREERDP_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			"lib"
			"VC"
			"lib/freerdp"
		)

	endif(MSVC)
elseif(UNIX)
	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
	find_library(LIB_FREERDP_CORE
			NAMES
			 freerdp-core
			HINTS
			 ${_FREERDP_LIBDIR}
			${_FREERDP_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			 lib
			)
	find_library(LIB_FREERDP_CODEC
			NAMES
			 freerdp-codec
			HINTS
			 ${_FREERDP_LIBDIR}
			${_FREERDP_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			 lib
			)
	find_library(LIB_FREERDP_GDI
			NAMES
			 freerdp-gdi
			HINTS
			 ${_FREERDP_LIBDIR}
			${_FREERDP_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			 lib
			)
	find_library(LIB_FREERDP_CACHE
			NAMES
			 freerdp-cache
			HINTS
			 ${_FREERDP_LIBDIR}
			${_FREERDP_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			 lib
			)
endif(WIN32 AND NOT CYGWIN)

mark_as_advanced(LIB_FREERDP_CORE LIB_FREERDP_CODEC LIB_FREERDP_GDI LIB_FREERDP_CACHE)

set(FREERDP_LIBRARIES ${LIB_FREERDP_CORE} ${LIB_FREERDP_CODEC} ${LIB_FREERDP_GDI} ${LIB_FREERDP_CACHE})

if (FREERDP_INCLUDE_DIR)
	if(_FREERDP_VERSION)
		set(FREERDP_VERSION "${_FREERDP_VERSION}")
	elseif(FREERDP_INCLUDE_DIR AND EXISTS "${FREERDP_INCLUDE_DIR}/freerdp/version.h")
		file(STRINGS "${FREERDP_INCLUDE_DIR}/freerdp/version.h" _freerdp_major_version
		REGEX "^#define[\t ]+FREERDP_VERSION_MAJOR[\t ]+[0-9]")

		string(REGEX REPLACE "^#define[\t ]+FREERDP_VERSION_MAJOR[\t ]+" "" 
			FREERDP_MAJOR_VERSION ${_freerdp_major_version})

		file(STRINGS "${FREERDP_INCLUDE_DIR}/freerdp/version.h" _freerdp_minor_version
		REGEX "^#define[\t ]+FREERDP_VERSION_MINOR[\t ]+[0-9]")

		string(REGEX REPLACE "^#define[\t ]+FREERDP_VERSION_MINOR[\t ]+" "" 
			FREERDP_MINOR_VERSION ${_freerdp_minor_version})

		file(STRINGS "${FREERDP_INCLUDE_DIR}/freerdp/version.h" _freerdp_revision_version
		REGEX "^#define[\t ]+FREERDP_VERSION_REVISION[\t ]+[0-9]")

		string(REGEX REPLACE "^#define[\t ]+FREERDP_VERSION_REVISION[\t ]+" "" 
			FREERDP_REVISION_VERSION ${_freerdp_revision_version})

		SET(FREERDP_VERSION "${FREERDP_MAJOR_VERSION}.${FREERDP_MINOR_VERSION}.${FREERDP_REVISION_VERSION}")
	endif(_FREERDP_VERSION)
endif(FREERDP_INCLUDE_DIR)

if(FREERDP_VERSION)
	find_package_handle_standard_args(FreeRDP
		REQUIRED_VARS
		 FREERDP_LIBRARIES
		 FREERDP_INCLUDE_DIR
		 VERSION_VAR
		  FREERDP_VERSION
		 FAIL_MESSAGE
		 "Could NOT find FreeRDP, try to set the path to FreeRDP root folder in the system variable FREERDP_ROOT_DIR"
	)
else()
	find_package_handle_standard_args(FreeRDP "Could NOT find FreeRDP, try to set the path to FreeRDP root folder in the system variable FREERDP_ROOT_DIR"
		FREERDP_LIBRARIES
		FREERDP_INCLUDE_DIR
	)
endif(FREERDP_VERSION)

mark_as_advanced(FREERDP_INCLUDE_DIR FREERDP_LIBRARIES)