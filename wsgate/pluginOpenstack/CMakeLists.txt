#pluginOpenstack

cmake_minimum_required(VERSION 2.8)

project("pluginOpenstack")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../cmake/)

# find casablanca
find_package(Casablanca REQUIRED)
include_directories(${CASABLANCA_INCLUDE_DIR})
# add casablanca to libs list
set(LIBS ${LIBS} ${CASABLANCA_LIBRARIES})

# search for boost libs
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
set(BOOST_ALL_DYN_LINK ON)
set(BOOST_ALL_NO_LIB ON)
find_package(Boost 1.48 REQUIRED)

set(HAVE_BOOST 1)

include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})
set(LIBS ${LIBS} ${Boost_LIBRARIES})

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

# create config.h from config.h.cmake
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
		${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(PLUGINOPENSTACK_SOURCE nova_token_auth.cpp pluginOpenstack.cpp ../pluginCommon.cpp)

if(WIN32)
    set(PLUGINOPENSTACK_SOURCE nova_token_auth.hpp ../pluginCommon.hpp)
endif()

add_library(pluginOpenstack SHARED ${PLUGINOPENSTACK_SOURCE})
