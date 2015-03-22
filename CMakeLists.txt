cmake_minimum_required (VERSION 2.6)
project (vcap C)

cmake_policy(SET CMP0015 NEW)

set(LIBRARY_OUTPUT_PATH lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY examples)

# Documentation
find_package(Doxygen)

if(DOXYGEN_FOUND)
	add_custom_target(doc
        COMMAND ${DOXYGEN_EXECUTABLE} doxygen.conf
        WORKING_DIRECTORY .
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
	set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "doc") 
endif(DOXYGEN_FOUND)

# Library

include_directories(include)

add_definitions(-Wall -std=c99 -D_GNU_SOURCE)
add_library(vcap SHARED "src/vcap.c" "src/vcap_decode.c")

# Examples

link_directories(lib)

add_executable(info "examples/info.c")
target_link_libraries (info vcap)
