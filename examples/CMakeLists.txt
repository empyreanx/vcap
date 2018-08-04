set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(BUILD_GRAB_EXAMPLE)
    add_subdirectory(grab)
endif()

if(BUILD_INFO_EXAMPLE)
    add_subdirectory(info)
endif()

if(BUILD_PNG_EXAMPLE)
    add_subdirectory(png)
endif()

if(BUILD_SDL_EXAMPLE)
    add_subdirectory(sdl)
endif()

link_directories(${CMAKE_BINARY_DIR}/lib)
