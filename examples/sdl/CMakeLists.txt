# SDL example
find_package(SDL)

#if(SDL_FOUND)
	include_directories(${PROJECT_SOURCE_DIR} ${SDL2_INCLUDE_DIR} ${V4L2_INCLUDE_DIRS})
	add_executable(sdl "sdl.c")
	target_link_libraries(sdl vcap SDL2 ${V4L2_LIBRARIES})
#else(SDL_FOUND)
#	message(WARNING "You need SDL2 installed in order to build examples/sdl.c")
#endif(SDL_FOUND)
