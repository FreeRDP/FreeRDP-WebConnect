# - Find Windows Platform SDK
# Find the Windows includes
# 
#  WINSDK_INCLUDE_DIR - where to find Windows.h
#  WINSDK_INCLUDE_DIRS - where to find all Windows headers
#  WINSDK_LIBRARY_DIR - where to find libraries
#  WINSDK_FOUND       - True if Windows SDK found.
#  WINSDK_SIGNTOOL    - where to find the sign toll
#  
#  Only WINSDK_INCLUDE_DIR is mandatory. All the rest variables need to be checked before use.
#  You can set the default path to the windows sdk using WINSDK_ROOT_DIR and the script 
#  will search that folder first for the necessary headers.

#  Note : list of windows sdk : https://en.wikipedia.org/wiki/Microsoft_Windows_SDK


 SET(HOST_CPU ${CMAKE_HOST_SYSTEM_PROCESSOR})

  IF(HOST_CPU MATCHES "amd64")
    SET(HOST_CPU "x64")
  ELSEIF(HOST_CPU MATCHES "i.86")
    SET(HOST_CPU "x86")
  ENDIF(HOST_CPU MATCHES "amd64")


set(_WINSDK_ROOT_HINTS
  ${WINSDK_ROOT_DIR}
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v8.1A;InstallationFolder]"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.1A;InstallationFolder]"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v8.1;InstallationFolder]"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v8.1;InstallationFolder]"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\v7.1;InstallationFolder]"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v7.1;InstallationFolder]"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows;CurrentInstallFolder]"
  "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\Microsoft\\Microsoft SDKs\\Windows;CurrentInstallFolder]"
  ENV WINSDK_ROOT_DIR
)

FILE(TO_CMAKE_PATH "$ENV{PROGRAMFILES}" _programfiles)

set(_WINSDK_ROOT_PATHS
  "${_programfiles}/Windows Kits/8.1"
  "${_programfiles}/Windows Kits/8.0"
  "${_programfiles}/Windows Kits/7.1"
  "${_programfiles}/Microsoft SDKs"
)

unset(_programfiles)

set(_WINSDK_ROOT_HINTS_AND_PATHS
    HINTS ${_WINSDK_ROOT_HINTS}
    PATHS ${_WINSDK_ROOT_PATHS}
  )

find_path(WINSDK_INCLUDE_DIR
          NAMES
           Windows.h
           WinSock2.h
           ${_WINSDK_ROOT_HINTS_AND_PATHS}
          PATH_SUFFIXES
           Include/um
           Include
)

find_path(WINSDK_SHARED_INCLUDE_DIR
          d3d9.h
           ${_WINSDK_ROOT_HINTS_AND_PATHS}
          PATH_SUFFIXES
           Include/shared
           Include
)

mark_as_advanced(WINSDK_SHARED_INCLUDE_DIR)

set(WINSDK_INCLUDE_DIRS ${WINSDK_INCLUDE_DIR} ${WINSDK_SHARED_INCLUDE_DIR})

set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")

find_path(WINSDK_LIBRARY_DIR
          NAMES
           ComCtl32.lib
           ${_WINSDK_ROOT_HINTS_AND_PATHS}
          PATH_SUFFIXES
           lib
           lib/win8/um
           lib/winv6.3/um
           lib/winv8/um
           lib/win7/um
           lib/winv7.1/um
           lib/${HOST_CPU}
           lib/win8/um/${HOST_CPU}
           lib/winv6.3/um/${HOST_CPU}
           lib/winv8/um/${HOST_CPU}
           lib/win7/um/${HOST_CPU}
           lib/winv7.1/um/${HOST_CPU}
)

mark_as_advanced(WINSDK_LIBRARY_DIR)


find_program(WINSDK_SIGNTOOL
            signtool
            HINTS
             ${_WINSDK_INCLUDEDIR}
             ${_WINSDK_ROOT_HINTS_AND_PATHS}
            PATH_SUFFIXES
             Bin
             Bin/${HOST_CPU}
)

mark_as_advanced(WINSDK_SIGNTOOL)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WinSDK
      REQUIRED_VARS
       WINSDK_INCLUDE_DIR
       FAIL_MESSAGE
       "Could NOT find Windows SDK, try to set the path to the WinSDK root folder in the system variable WINSDK_ROOT_DIR"
    )