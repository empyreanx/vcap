cmake_minimum_required (VERSION 2.6)

# Prevent in source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project (vcap C)

# Declare options
option(BUILD_GRAB_EXAMPLE "Build an example that grabs and saves a frame" OFF)
option(BUILD_INFO_EXAMPLE "Build an example that dumps info about a camera to stdout" OFF)
option(BUILD_JPEG_EXAMPLE "Build an example that grabs and saves a frame to a PNG file" OFF)
option(BUILD_PNG_EXAMPLE "Build an example that grabs and saves a frame to a JPEG file" OFF)
option(BUILD_SDL_EXAMPLE "Build an example that streams frames using SDL 1.2" OFF)
option(BUILD_JSON_EXAMPLE "Buid an example that exports/imports device settings" OFF)
option(SUPPORT_JSON "Input/export camera settings to/from JSON" ON)
option(SUPPORT_PNG "Save frames to PNG" ON)
option(SUPPORT_JPEG "Save frames to JPEG" ON)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Add include directory
include_directories(include)

# Add definitions
if(NOT DEFINED ${CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_definitions(-O2)
else()
    add_definitions(-g)
endif()

add_definitions(-Wall -std=c99 -D_GNU_SOURCE)

# Add souce files
file(GLOB SOURCE_FILES
    include/vcap/vcap.h
    src/*.h
    src/*.c
)

set(LIBRARY_OUTPUT_PATH lib)

# Add V4L2 library
add_library(vcap SHARED ${SOURCE_FILES})

# Find V4L2
find_package(V4L2 REQUIRED)

if(NOT V4L2_FOUND)
    message(FATAL_ERROR "V4L2 not found!")
endif()

target_link_libraries(vcap ${V4L2_LIBRARIES})
target_include_directories(vcap PUBLIC ${V4L2_INCLUDE_DIR})

# Add examples
add_subdirectory(examples)

# Installation
install(DIRECTORY include/vcap DESTINATION include)
install(TARGETS vcap DESTINATION lib)

# Documentation
find_package(Doxygen)

if(DOXYGEN_FOUND)
    configure_file(${PROJECT_SOURCE_DIR}/include/vcap/vcap.h ${CMAKE_CURRENT_BINARY_DIR}/vcap.h COPYONLY)

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/doxygen.conf
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "doc")
endif()
