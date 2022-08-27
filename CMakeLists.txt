cmake_minimum_required (VERSION 2.6)

# Prevent in source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

project (vcap C)

# Declare options
option(BUILD_SHARED       "Build a shared library" ON)
option(BUILD_STATIC       "Build a static library" OFF)
option(BUILD_GRAB_EXAMPLE "Build an example that grabs and saves a frame" OFF)
option(BUILD_INFO_EXAMPLE "Build an example that prints camera info" OFF)
option(BUILD_SDL_EXAMPLE  "Build an example that streams frames using SDL2" OFF)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Add definitions
if(NOT DEFINED ${CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_definitions(-O2)
else()
    add_definitions(-g)
endif()

add_definitions(-std=c99 -Wall -Wextra -pedantic -D_GNU_SOURCE) #-pedantic

# Add souce files
file(GLOB SOURCE_FILES
    vcap.h
    vcap.c
)

set(LIBRARY_OUTPUT_PATH lib)

# Build shared library
if(BUILD_SHARED)
    add_library(vcap SHARED ${SOURCE_FILES})
endif()

# Build static library
if(BUILD_STATIC)
    add_library(vcap-static STATIC ${SOURCE_FILES})
endif()

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
install(FILES vcap.h DESTINATION include)
install(TARGETS vcap DESTINATION lib)

# Documentation
find_package(Doxygen)

if(DOXYGEN_FOUND)
    configure_file(${PROJECT_SOURCE_DIR}/vcap.h ${CMAKE_CURRENT_BINARY_DIR}/vcap.h COPYONLY)

    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "docs")
endif()
