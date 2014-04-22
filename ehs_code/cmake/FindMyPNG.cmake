# - Find png
# Find the native png includes and library.
# Once done this will define
#
#  PNG_INCLUDE_DIRS   - where to find png.h, etc.
#  PNG_LIBRARIES      - List of libraries when using png.
#  PNG_FOUND          - True if png found.
#  PNG_LIBRARY
#  
#
# An includer may set PNG_ROOT to a png installation root to tell
# this module where to look.

#=============================================================================
# Copyright 2001-2011 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

# Modified by Mario Reja (mreja@cloudbasesolutions.com) by adding win32-specific checks.


set(_PNG_SEARCHES)

# Search PNG_ROOT first if it is set.
if(PNG_ROOT)
  set(_PNG_SEARCH_ROOT PATHS ${PNG_ROOT} NO_DEFAULT_PATH)
  list(APPEND _PNG_SEARCHES _PNG_SEARCH_ROOT)
endif()

# Normal search.
set(_PNG_SEARCH_NORMAL
  PATHS "[HKEY_LOCAL_MACHINE\\SOFTWARE\\GnuWin32\\LibPng;InstallPath]"
        "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Wow6432Node\\GnuWin32\\LibPng;InstallPath]"
        "$ENV{PROGRAMFILES}/png"
        "$ENV{PROGRAMFILES}/LibPng"
  )
list(APPEND _PNG_SEARCHES _PNG_SEARCH_NORMAL)

set(PNG_NAMES png libpng)

# Try each search configuration.
SET(CMAKE_FIND_LIBRARY_PREFIXES "")
SET(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" ".a")
foreach(search ${_PNG_SEARCHES})
  find_path(PNG_INCLUDE_DIR NAMES png.h        ${${search}} PATH_SUFFIXES include)
  find_library(PNG_LIBRARY  NAMES ${PNG_NAMES} ${${search}} PATH_SUFFIXES lib)
endforeach()

if(PNG_INCLUDE_DIR AND EXISTS "${PNG_INCLUDE_DIR}/png.h")
    file(STRINGS "${PNG_INCLUDE_DIR}/png.h" _png_major_version
        REGEX "#define[\t ]+PNG_LIBPNG_VER_MAJOR[\t ]+[0-9]")

    string(REGEX REPLACE "^#define[\t ]+PNG_LIBPNG_VER_MAJOR[\t ]+" "" 
      PNG_MAJOR_VERSION ${_png_major_version})

    file(STRINGS "${PNG_INCLUDE_DIR}/png.h" _png_minor_version
        REGEX "#define[\t ]+PNG_LIBPNG_VER_MINOR[\t ]+[0-9]+")

    string(REGEX REPLACE "^#define[\t ]+PNG_LIBPNG_VER_MINOR[\t ]+" "" 
      PNG_MINOR_VERSION ${_png_minor_version})

    file(STRINGS "${PNG_INCLUDE_DIR}/png.h" _png_release_version
        REGEX "#define[\t ]+PNG_LIBPNG_VER_RELEASE[\t ]+[0-9]+")

    string(REGEX REPLACE "^#define[\t ]+PNG_LIBPNG_VER_RELEASE[\t ]+" "" 
      PNG_RELEASE_VERSION ${_png_release_version})

    set(PNG_VERSION_STRING "${PNG_MAJOR_VERSION}.${PNG_MINOR_VERSION}.${PNG_RELEASE_VERSION}")

endif()

# handle the QUIETLY and REQUIRED arguments and set PNG_FOUND to TRUE if
# all listed variables are TRUE
set(PNG_LIBRARIES ${PNG_LIBRARY})

FIND_PACKAGE_HANDLE_STANDARD_ARGS(MyPNG REQUIRED_VARS PNG_LIBRARIES PNG_INCLUDE_DIR
                                       		 FAIL_MESSAGE
		 "Could NOT find PNG, try to set the path to PNG root folder in the system variable PNG_ROOT_DIR"
)


mark_as_advanced(PNG_INCLUDE_DIR PNG_LIBRARY)