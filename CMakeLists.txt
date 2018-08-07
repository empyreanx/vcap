cmake_minimum_required (VERSION 2.6)

# Prevent in source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project (vcap C)

# Declare options
option(BUILD_GRAB_EXAMPLE "Build an example that grabs and saves a frame" OFF)
option(BUILD_INFO_EXAMPLE "Build an example that dumps info about a camera to stdout" OFF)
option(BUILD_PNG_EXAMPLE "Build an example that grabs and saves a frame to a png file" OFF)
option(BUILD_SDL_EXAMPLE "Build an example that streams frames using SDL 1.2" OFF)
option(BUILD_DOCS "Build Doxygen docs" OFF)
option(SUPPORT_JSON "Input/export camera settings to/from JSON" ON)
option(SUPPORT_PNG "Save frames to PNG" ON)
option(SUPPORT_JPEG "Save frames to JPEG" ON)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Add include directory
include_directories(include)

# Add definitions
set(CMAKE_BUILD_TYPE Release)

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_definitions(-O2)
endif()

add_definitions(-Wall -std=c99 -D_GNU_SOURCE)

# Add souce files
file(GLOB SOURCE_FILES
    include/vcap/*.h
    src/*.h
    src/*.c
)

# Find V4L2
find_package(V4L2 REQUIRED)

if(NOT V4L2_FOUND)
    message(FATAL_ERROR "V4L2 not found!")
endif()

include_directories(${V4L2_INCLUDE_DIR})

# Find Jansson
if(SUPPORT_JSON)
    find_package(Jansson)

    if(NOT JANSSON_FOUND)
        message(WARNING "Unable to find Jansson: disabling JSON support")
    else()
        add_definitions(-DVCAP_SUPPORT_JSON)
        include_directories(JANSSON_INCLUDE_DIRS)
    endif()
endif()

# Find libpng
if(SUPPORT_PNG)
    find_package(PNG)

    if(NOT PNG_FOUND)
        message(WARNING "Unable to find libpng: disabling PNG support")
    else()
        add_definitions(-DVCAP_SUPPORT_PNG)
        include_directories(PNG_INCLUDE_DIRS)
    endif()
endif()

# Find libjpeg
if(SUPPORT_JPEG)
    find_package(JPEG)

    if(NOT JPEG_FOUND)
        message(WARNING "Unable to find libjpeg: disabling JPEG support")
    else()
        add_definitions(-DVCAP_SUPPORT_JPEG)
        include_directories(JPEG_INCLUDE_DIR)
    endif()
endif()

# Add library
set(LIBRARY_OUTPUT_PATH lib)
add_library(vcap SHARED ${SOURCE_FILES})

# Add examples
add_subdirectory(examples)

# Installation
install(DIRECTORY include/vcap DESTINATION include)
install(TARGETS vcap DESTINATION lib)

# Documentation
if(BUILD_DOCS)
    find_package(Doxygen)

    if(NOT DOXYGEN_FOUND)
        message(FATAL_ERROR "Doxygen not found!")
    endif()

    configure_file(${PROJECT_SOURCE_DIR}/include/vcap/vcap.h ${CMAKE_CURRENT_BINARY_DIR}/vcap.h COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/include/vcap/ctrls.h ${CMAKE_CURRENT_BINARY_DIR}/ctrls.h COPYONLY)
    configure_file(${PROJECT_SOURCE_DIR}/include/vcap/fmts.h ${CMAKE_CURRENT_BINARY_DIR}/fmts.h COPYONLY)

	add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/doxygen.conf
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

	set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "doc")
endif()
