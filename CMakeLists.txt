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
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib)

include_directories(include)

add_definitions(-Wall -std=c99 -D_GNU_SOURCE)
add_library(vcap SHARED "src/vcap.c" "src/decode.c" "src/formats.c")

# Examples
link_directories(lib)

# Info example
add_executable(info "examples/info.c")
target_link_libraries (info vcap)

# Grab example
add_executable(grab "examples/grab.c")
target_link_libraries (grab vcap)

# PNG example
find_package(PNG)

if(PNG_FOUND)
	include_directories(${PNG_INCLUDE_DIR})
	add_executable(png "examples/png.c")
	target_link_libraries(png vcap ${PNG_LIBRARY})
else(PNG_FOUND)
	message(WARNING "You need libPNG install in order to build examples/png.c")
endif(PNG_FOUND)

# SDL example
find_package(SDL)

if(SDL_FOUND)
	include_directories(${SDL_INCLUDE_DIR})
	add_executable(sdl "examples/sdl.c")
	target_link_libraries(sdl vcap ${SDL_LIBRARY})
else(SDL_FOUND)
	message(WARNING "You need SDL1.2 install in order to build examples/sdl.c")
endif(SDL_FOUND)

# Installation

install(DIRECTORY include/vcap DESTINATION include)
install(FILES lib/libvcap.so DESTINATION lib)
