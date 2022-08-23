# SDL example
find_package(SDL2)

if(SDL2_FOUND)
	include_directories(${PROJECT_SOURCE_DIR} ${SDL2_INCLUDE_DIR} ${V4L2_INCLUDE_DIRS})
	add_executable(sdl "sdl.c")
	target_link_libraries(sdl vcap SDL2 ${V4L2_LIBRARIES})
else(SDL2_FOUND)
	message(WARNING "You need SDL2 installed in order to build examples/sdl.c")
endif(SDL2_FOUND)
