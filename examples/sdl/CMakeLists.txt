# SDL example
find_package(SDL)

if(SDL_FOUND)
	include_directories(${SDL_INCLUDE_DIR})
	add_executable(sdl "sdl.c")
	target_link_libraries(sdl vcap ${SDL_LIBRARY} ${LIBRARIES})
else(SDL_FOUND)
	message(WARNING "You need SDL1.2 install in order to build examples/sdl.c")
endif(SDL_FOUND)
