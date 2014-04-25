# FindEHS package
# 
# Set EHS_ROOT_DIR to guide the package to the casablanca installation.
# The script will set EHS_INCLUDE_DIR pointing to the location of header files.
# and EHS_LIBRARIES will be a list that can be passed to taget_link_libraries.
# The script checks for the ehs/ehs.h header file only.
# It searches for libehs.lib ehs.lib ehs.a libehs.so and libehs.a,
# But will only add one of them to the link command.

if(UNIX)
	find_package(PkgConfig)
	pkg_check_modules(_EHS QUIET ehs)
endif()

if (WIN32)
	set(_EHS_ROOT_HINTS
		${EHS_ROOT_DIR}
		ENV EHS_ROOT_DIR
	)
	file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)
	set(_EHS_ROOT_PATHS
		"${_programfiles}/EHS"
		"C:/EHS"
	)
	unset(_programfiles)
	set(_EHS_ROOT_HINTS_AND_PATHS
		HINTS ${_EHS_ROOT_HINTS}
		PATHS ${_EHS_ROOT_PATHS}
	)
else()
	set(_EHS_ROOT_HINTS
    		${EHS_ROOT_DIR}
    		ENV EHS_ROOT_DIR
    	)
endif(WIN32)

find_path(EHS_INCLUDE_DIR
	NAMES
	 ehs.h
	 ehs/ehs.h
	 HINTS
	 ${_EHS_INCLUDEDIR}
	 ${_EHS_ROOT_HINTS_AND_PATHS}
	 PATH_SUFFIXES
	 include
)

if(WIN32 AND NOT CYGWIN)
	if(MSVC)
		SET(CMAKE_FIND_LIBRARY_PREFIXES "")
		SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" ".a")
		
		find_library(LIB_EHS
			NAMES
			 libehs
			 ehs
			${_EHS_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			"lib"
			"VC"
		)
	endif(MSVC)
elseif(UNIX)
	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
	find_library(LIB_EHS
			NAMES
			 ehs
			HINTS
			 ${_EHS_LIBDIR}
			${_EHS_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			 lib
			)
endif(WIN32 AND NOT CYGWIN)

set(EHS_LIBRARIES ${LIB_EHS})

#if (EHS_INCLUDE_DIR)
#	if(_EHS_VERSION)
#		set(EHS_VERSION "${_EHS_VERSION}")
#	elseif(EHS_INCLUDE_DIR AND EXISTS "${EHS_INCLUDE_DIR}/EHS/version.h")
#		file(STRINGS "${EHS_INCLUDE_DIR}/EHS/version.h" _EHS_major_version
#		REGEX "^#define[\t ]+EHS_VERSION_MAJOR[\t ]+[0-9]")
#
#		string(REGEX REPLACE "^#define[\t ]+EHS_VERSION_MAJOR[\t ]+" "" 
#			EHS_MAJOR_VERSION ${_EHS_major_version})
#
#		file(STRINGS "${EHS_INCLUDE_DIR}/EHS/version.h" _EHS_minor_version
#		REGEX "^#define[\t ]+EHS_VERSION_MINOR[\t ]+[0-9]")
#
#		string(REGEX REPLACE "^#define[\t ]+EHS_VERSION_MINOR[\t ]+" "" 
#			EHS_MINOR_VERSION ${_EHS_minor_version})
#
#		file(STRINGS "${EHS_INCLUDE_DIR}/EHS/version.h" _EHS_revision_version
#		REGEX "^#define[\t ]+EHS_VERSION_REVISION[\t ]+[0-9]")
#
#		string(REGEX REPLACE "^#define[\t ]+EHS_VERSION_REVISION[\t ]+" "" 
#			EHS_REVISION_VERSION ${_EHS_revision_version})
#
#		SET(EHS_VERSION "${EHS_MAJOR_VERSION}.${EHS_MINOR_VERSION}.${EHS_REVISION_VERSION}")
#	endif(_EHS_VERSION)
#endif(EHS_INCLUDE_DIR)

# ehs provides no versioning info inside its header files
# but because it is no longer under active development, it is most probably version 1.5.0


if(EHS_VERSION)
	find_package_handle_standard_args(EHS
		REQUIRED_VARS
		 EHS_LIBRARIES
		 EHS_INCLUDE_DIR

		 FAIL_MESSAGE
		 "Could NOT find EHS, try to set the path to EHS root folder in the system variable EHS_ROOT_DIR"
	)
else()
	find_package_handle_standard_args(EHS "Could NOT find EHS, try to set the path to EHS root folder in the system variable EHS_ROOT_DIR"
		EHS_LIBRARIES
		EHS_INCLUDE_DIR
	)
endif(EHS_VERSION)

mark_as_advanced(EHS_INCLUDE_DIR EHS_LIBRARIES)