set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(BUILD_CAPTURE_EXAMPLE)
    add_subdirectory(capture)
endif()

if(BUILD_INFO_EXAMPLE)
    add_subdirectory(info)
endif()

if(BUILD_SDL_EXAMPLE)
    add_subdirectory(sdl)
endif()

link_directories(${CMAKE_BINARY_DIR}/lib)
