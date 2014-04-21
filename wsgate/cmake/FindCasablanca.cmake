# FindCasablanca package
#
# Set CASABLANCA_ROOT_DIR to guide the package to the casablanca installation.
# The script will set CASABLANCA_INCLUDE_DIR pointing to the location of header files.
# and CASABLANCA_LIBRARIES will be a list that can be passed to taget_link_libraries.
# The script checks for cpprest/http_client.h and cpprest/json.h header files only.
# It searches for casablanca.lib casablanca120.lib libcasablanca.so and libcasablanca.a,
# But will only add one of them to the link list.

if(UNIX)
	find_package(PkgConfig)
	pkg_check_modules(_CASABLANCA QUIET casablanca)
endif()

if (WIN32)
	set(_CASABLANCA_ROOT_HINTS
		${CASABLANCA_ROOT_DIR}
		"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6443Node\\Microsoft\\Casablanca\\120\\SDK;InstallDir]"
		"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Casablanca\\120\\SDK;InstallDir]"
		"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6443Node\\Microsoft\\CppRestSDK\\OpenSourceRelease\\120\\v1.4\\SDK;InstallDir]"
		"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\CppRestSDK\\OpenSourceRelease\\120\\v1.4\\SDK;InstallDir]"
		# newer versions are not searched, because the project abandoned global installer support
		#"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6443Node\\Microsoft\\CppRestSDK\OpenSourceRelease\120\v2.0\SDK;InstallDir]"
		ENV CASABLANCA_ROOT_DIR
	)
	file(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)
	set(_CASABLANCA_ROOT_PATHS
		"${_programfiles}/Casablanca"
		"${_programfiles}/Microsoft SDKs"
		"${_programfiles}/Microsoft SDKs/Cpp REST SDK for Visual Studio 2010"
		"${_programfiles}/Microsoft SDKs/Cpp REST SDK for Visual Studio 2010/SDK"
		"${_programfiles}/Microsoft SDKs/Cpp REST SDK for Visual Studio 2012"
		"${_programfiles}/Microsoft SDKs/Cpp REST SDK for Visual Studio 2012/SDK"
		"${_programfiles}/Microsoft SDKs/Cpp REST SDK for Visual Studio 2013"
		"${_programfiles}/Microsoft SDKs/Cpp REST SDK for Visual Studio 2013/SDK"
	)
	unset(_programfiles)
	set(_CASABLANCA_ROOT_HINTS_AND_PATHS
		HINTS ${_CASABLANCA_ROOT_HINTS}
		PATHS ${_CASABLANCA_ROOT_PATHS}
	)
else()
	set(_CASABLANCA_ROOT_HINTS
    		${CASABLANCA_ROOT_DIR}
    		ENV CASABLANCA_ROOT_DIR
    	)
endif(WIN32)

find_path(CASABLANCA_INCLUDE_DIR
	NAMES
	 cpprest/http_client.h
	 cpprest/json.h
	 HINTS
	 ${_CASABLANCA_ROOT_HINTS_AND_PATHS}
	 PATH_SUFFIXES
	  include
	  include/cpprest
	  include/pplx
	  include/compat
	  include/casablanca
	  include/casablanca/cpprest
	  include/casablanca/pplx
	  include/casablanca/compat
)

if(WIN32 AND NOT CYGWIN)
	if(MSVC)
		SET(CMAKE_FIND_LIBRARY_PREFIXES "")
		# check msvc version and add to suffix list
		set(MSVC_SUFFIX)
		if(MSVC12)
			set(MSVC_SUFFIX "120_2_0.lib")
		elseif(MSVC11)
			set(MSVC_SUFFIX "110.lib")
		elseif(MSVC10)
			set(MSVC_SUFFIX "100.lib")
		elseif(MSVC90)
			set(MSVC_SUFFIX "90.lib")
		elseif(MSVC80)
			set(MSVC_SUFFIX "80.lib")
		elseif(MSVC71)
			set(MSVC_SUFFIX "71.lib")
		elseif(MSVC11)
			set(MSVC_SUFFIX "70.lib")
		endif()

		SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" "${MSVC_SUFFIX}")

		if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
			set(BITNESS "x86")
		else()
			set(BITNESS "x64")
		endif()

		find_library(LIB_CASABLANCA
			NAMES
			 cpprest
			${_CASABLANCA_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			"lib"
			"VC"
			"lib/casablanca"
			"lib/Release"
			"lib/${BITNESS}/Release"
		)

	endif(MSVC)
elseif(UNIX)
	SET(CMAKE_FIND_LIBRARY_PREFIXES "lib")
	SET(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
	find_library(LIB_CASABLANCA
			NAMES
			 casablanca
			HINTS
			 ${_CASABLANCA_LIBDIR}
			${_CASABLANCA_ROOT_HINTS_AND_PATHS}
			PATH_SUFFIXES
			 lib
			)
endif(WIN32 AND NOT CYGWIN)

mark_as_advanced(LIB_CASABLANCA)

set(CASABLANCA_LIBRARIES ${LIB_CASABLANCA})

if (CASABLANCA_INCLUDE_DIR)
	if(_CASABLANCA_VERSION)
		set(CASABLANCA_VERSION "${_CASABLANCA_VERSION}")
	elseif(CASABLANCA_INCLUDE_DIR AND EXISTS "${CASABLANCA_INCLUDE_DIR}/version.h")
		file(STRINGS "${CASABLANCA_INCLUDE_DIR}/freerdp/version.h" _casablanca_major_version
		REGEX "^#define[\t ]+CPPREST_VERSION_MAJOR[\t ]+[0-9]")

		string(REGEX REPLACE "^#define[\t ]+CPPREST_VERSION_MAJOR[\t ]+" "" 
			CASABLANCA_MAJOR_VERSION ${_casablanca_major_version})

		file(STRINGS "${CASABLANCA_INCLUDE_DIR}/version.h" _casablanca_minor_version
		REGEX "^#define[\t ]+CPPREST_VERSION_MINOR[\t ]+[0-9]")

		string(REGEX REPLACE "^#define[\t ]+CPPREST_VERSION_MINOR[\t ]+" "" 
			CASABLANCA_MINOR_VERSION ${_casablanca_minor_version})

		file(STRINGS "${CASABLANCA_INCLUDE_DIR}/version.h" _casablanca_revision_version
		REGEX "^#define[\t ]+CPPREST_VERSION_REVISION[\t ]+[0-9]")

		string(REGEX REPLACE "^#define[\t ]+CPPREST_VERSION_REVISION[\t ]+" "" 
			CASABLANCA_REVISION_VERSION ${_casablanca_revision_version})

		SET(CASABLANCA_VERSION "${CASABLANCA_MAJOR_VERSION}.${CASABLANCA_MINOR_VERSION}.${CASABLANCA_REVISION_VERSION}")
	endif(_CASABLANCA_VERSION)
endif(CASABLANCA_INCLUDE_DIR)

if(CASABLANCA_LIBRARIES AND CASABLANCA_INCLUDE_DIR)
	if(CASABLANCA_VERSION)
		find_package_handle_standard_args(Casablanca
			REQUIRED_VARS
			 CASABLANCA_LIBRARIES
			 CASABLANCA_INCLUDE_DIR
			 VERSION_VAR
			  CASABLANCA_VERSION
			 FAIL_MESSAGE
			 "Could NOT find FreeRDP, try to set the path to Casablanca root folder in the system variable CASABLANCA_ROOT_DIR"
		)
	else()
		find_package_handle_standard_args(Casablanca
			REQUIRED_VARS
			 CASABLANCA_LIBRARIES
			 CASABLANCA_INCLUDE_DIR
			 FAIL_MESSAGE
			"Could NOT find FreeRDP, try to set the path to Casablanca root folder in the system variable CASABLANCA_ROOT_DIR"
		)
	endif(CASABLANCA_VERSION)
else()
	find_package_handle_standard_args(FreeRDP "Could NOT find Casablanca, try to set the path to Casablanca root folder in the system variable CASABLANCA_ROOT_DIR"
		CASABLANCA_LIBRARIES
		CASABLANCA_INCLUDE_DIR
	)
endif()

mark_as_advanced(CASABLANCA_INCLUDE_DIR CASABLANCA_LIBRARIES)