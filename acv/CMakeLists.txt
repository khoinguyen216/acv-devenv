cmake_minimum_required(VERSION 3.2)
project(acv CXX C)

include(user.cmake)

# System architecture
message(STATUS "System: ${CMAKE_SYSTEM_NAME}")
message(STATUS "CPU: ${CMAKE_SYSTEM_PROCESSOR}")

# Build options
option(WITH_GUI "Enable graphical user interface" OFF)
if(WITH_GUI)
	add_definitions(-DWITH_GUI)
endif()

# Set installation directory
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install"
	CACHE PATH "Installation directory")

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU"
	OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fpermissive")
endif()

# Qt5
find_package(Qt5 5.4.1 REQUIRED Widgets)
get_target_property(Qt5Core_LIBPATH Qt5::Core LOCATION)
get_filename_component(Qt5_LIBDIR ${Qt5Core_LIBPATH} DIRECTORY)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# OpenCV
FIND_PACKAGE ( OpenCV 2.4.10 REQUIRED )
GET_TARGET_PROPERTY ( OpenCVCore_LIBPATH opencv_core IMPORTED_LOCATION_${BUILD_TYPE} )
GET_FILENAME_COMPONENT ( OpenCV_LIBDIR ${OpenCVCore_LIBPATH} DIRECTORY )

# Boost
SET ( Boost_USE_STATIC_LIBS     ON )
SET ( Boost_USE_MULTITHREADED   ON )
SET ( Boost_NO_SYSTEM_PATHS     ON )
FIND_PACKAGE ( Boost 1.57.0
    COMPONENTS
        date_time filesystem system thread program_options
    REQUIRED )
ADD_DEFINITIONS ( -DBOOST_ALL_NO_LIB )

include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(algorithm)
add_subdirectory(app)
add_subdirectory(core)
add_subdirectory(module)
add_subdirectory(shared)